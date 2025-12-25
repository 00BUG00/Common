#pragma once

#include <coroutine>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <chrono>

/**
 * @class DefaultBackoffPolicy
 * @brief 默认退避策略（Spin → Yield → Sleep）
 *
 * @details
 * 用于 Runtime 在“无任务进展”时降低 CPU 占用。
 * 属于调度策略层，而非 Runtime 内核。
 */
class DefaultBackoffPolicy {
public:
    void operator()(size_t missCount) {
        if (missCount < 50) {
            // 短暂自旋
        } else if (missCount < 200) {
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(
                std::chrono::microseconds(50));
        }
    }
};

/**
 * @class CoroutineExecutorMT
 * @brief 多线程 + 多协程 Runtime（M:N 调度）
 *
 * @details
 * 设计目标：
 * - 多个 OS 线程
 * - 每线程多个协程
 * - 协程协作式调度
 * - 线程抢占式调度
 *
 * 职责边界：
 * - ❌ 不存储任务
 * - ❌ 不决定并发语义
 * - ❌ 不保证公平 / 顺序
 * - ✅ 只负责调度与执行
 *
 * @tparam T                任务类型
 * @tparam LockFreeQueue    无锁队列类型（如 RingQueue<T>）
 * @tparam BackoffPolicy    调度退避策略
 *
 * @author BUG
 * @date 2025-12-24
 */
template<
    typename T,
    typename LockFreeQueue,
    typename BackoffPolicy = DefaultBackoffPolicy
>
class CoroutineExecutorMT {
public:
    using Callback = std::function<void(const T&)>;

    /**
     * @brief 构造 Runtime
     *
     * @param queue                外部无锁任务队列
     * @param cb                   任务处理回调
     * @param threadCount          工作线程数量
     * @param coroutinePerThread   每线程协程数量
     */
    CoroutineExecutorMT(
        LockFreeQueue& queue,
        Callback cb,
        size_t threadCount,
        size_t coroutinePerThread)
        : _queue(queue)
        , _callback(std::move(cb))
        , _running(false)
        , _threadCount(threadCount)
        , _coroutinePerThread(coroutinePerThread)
    {}

    /**
     * @brief 析构时自动停止 Runtime
     */
    ~CoroutineExecutorMT() {
        Stop();
    }

    /**
     * @brief 启动 Runtime
     */
    void Start() {
        if (_running.exchange(true))
            return;

        for (size_t i = 0; i < _threadCount; ++i) {
            _threads.emplace_back(
                &CoroutineExecutorMT::ThreadMain, this);
        }
    }

    /**
     * @brief 停止 Runtime
     */
    void Stop() {
        if (!_running.exchange(false))
            return;

        _cv.notify_all();

        for (auto& t : _threads) {
            if (t.joinable())
                t.join();
        }
        _threads.clear();
    }

private:
    /**
     * @brief 协程封装类型
     */
    struct WorkerTask {
        struct promise_type {
            WorkerTask get_return_object() {
                return WorkerTask{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };

        std::coroutine_handle<promise_type> handle;
    };

    /**
     * @brief 协程执行循环
     *
     * 协作式语义：
     * - TryPop 成功 → 执行任务
     * - TryPop 失败 → 主动让出执行权
     */
    WorkerTask CoroutineLoop() {
        while (_running.load(std::memory_order_relaxed)) {
            T task;
            if (_queue.TryPop(task) == RingQueueResult::Ok) {
                _callback(task);
            } else {
                co_await std::suspend_always{};
            }
        }
        co_return;
    }

    /**
     * @brief 工作线程主函数
     *
     * 行为：
     * - 创建本线程的协程集合
     * - 轮询恢复协程
     * - 无进展时执行 Backoff
     */
    void ThreadMain() {
        std::vector<WorkerTask> tasks;
        tasks.reserve(_coroutinePerThread);

        for (size_t i = 0; i < _coroutinePerThread; ++i) {
            tasks.emplace_back(CoroutineLoop());
        }

        BackoffPolicy backoff;
        size_t missCount = 0;

        while (_running.load(std::memory_order_relaxed)) {
            bool progressed = false;

            for (auto& task : tasks) {
                if (task.handle && !task.handle.done()) {
                    task.handle.resume();
                    progressed = true;
                }
            }

            if (!progressed) {
                ++missCount;
                backoff(missCount);
            } else {
                missCount = 0;
            }
        }

        for (auto& task : tasks) {
            if (task.handle)
                task.handle.destroy();
        }
    }

private:
    LockFreeQueue& _queue;           ///< 外部无锁任务队列
    Callback _callback;              ///< 任务执行回调

    std::atomic<bool> _running;      ///< 运行状态

    size_t _threadCount;             ///< 工作线程数量
    size_t _coroutinePerThread;      ///< 每线程协程数量

    std::vector<std::thread> _threads;

    std::condition_variable _cv;
    std::mutex _mutex;
};
