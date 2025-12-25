#pragma once

#include <coroutine>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

/**
 * @class CoroutineConsumer
 * @brief 基于协程的多协程任务消费者
 * @details
 * CoroutineConsumer 使用一个线程作为协程调度器（EventLoop）：
 * - 多个消费者协程并发消费同一个任务队列
 * - 生产者通过 AddTask() 提交任务
 * - 协程在无任务时挂起，由 EventLoop 恢复
 *
 * @tparam T 任务类型
 * @author BUG
 * @date 2025-12-25
 */
template<typename T>
class CoroutineConsumer {
public:
    /// 任务处理回调类型
    using Callback = std::function<void(T)>;

    /**
     * @brief 构造协程消费者
     * @details 仅初始化内部状态，不启动线程或协程
     *
     * @param func 用户任务处理回调
     * @param coroutineCount 消费者协程数量
     * @author BUG
     * @date 2025-12-25
     */
    CoroutineConsumer(Callback func, int coroutineCount = 1)
        : _callback(std::move(func)), _running(false), _coroutineCount(coroutineCount)
    {}

    /**
     * @brief 析构函数
     * @details 调用 Stop()，确保线程退出，协程安全结束
     * @author BUG
     * @date 2025-12-25
     */
    ~CoroutineConsumer() {
        Stop();
    }

    /**
     * @brief 启动协程消费者系统
     * @details
     * - 创建 coroutineCount 个消费者协程
     * - 启动 EventLoop 线程作为调度器
     * - 重复调用无效
     *
     * @thread_safety 非线程安全，应在初始化阶段调用
     * @author BUG
     * @date 2025-12-25
     */
    void Start() {
        if (_running) return;
        _running = true;
        for (int i = 0; i < _coroutineCount; i++) {
            _coroutines.push_back(ConsumeCoroutine());
        }
        _worker = std::thread(&CoroutineConsumer::EventLoop, this);
    }

    /**
     * @brief 停止协程消费者系统
     * @details
     * - 设置运行状态为 false
     * - 唤醒 EventLoop
     * - 等待调度线程退出
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    void Stop() {
        if (!_running) return;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _running = false;
        }
        _cv.notify_all();
        if (_worker.joinable())
            _worker.join();
    }

    /**
     * @brief 添加任务到队列
     * @details
     * 将任务加入内部队列，通知 EventLoop 恢复等待协程
     *
     * @param task 要处理的任务
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    void AddTask(T task) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_running) return;
            _queue.push(std::move(task));
        }
        _cv.notify_one();
    }

    /**
     * @brief 获取当前任务队列大小
     * @return 队列中未处理任务数量
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    /**
     * @brief 消费者协程主函数
     * @details
     * 循环等待任务，co_await 挂起协程，无任务时不会阻塞线程
     * @return ConsumerTask 协程对象
     * @author BUG
     * @date 2025-12-25
     */
    struct ConsumerTask {
        struct promise_type {
            ConsumerTask get_return_object() {
                return ConsumerTask{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };
        std::coroutine_handle<promise_type> h; ///< 协程句柄
    };

    /**
     * @brief 协程执行函数
     * @details 每个协程循环取任务执行回调
     * @return ConsumerTask 协程对象
     * @author BUG
     * @date 2025-12-25
     */
    ConsumerTask ConsumeCoroutine() {
        while (true) {
            T task;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [&] { return !_queue.empty() || !_running; });
                if (!_running && _queue.empty()) co_return;
                task = std::move(_queue.front());
                _queue.pop();
            }
            _callback(std::move(task));
            co_await std::suspend_always{};
        }
    }

    /**
     * @brief 协程调度线程
     * @details 唤醒等待中的协程，保持协程调度系统运行
     * @author BUG
     * @date 2025-12-25
     */
    void EventLoop() {
        while (true) {
            std::coroutine_handle<> waiter;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                if (!_running && _queue.empty() && _waiters.empty()) break;
                _cv.wait(lock, [&]{ return !_queue.empty() || !_running; });
                if (_waiters.empty()) continue;
                waiter = _waiters.front();
                _waiters.pop();
            }
            if (waiter) waiter.resume();
        }

        while (!_waiters.empty()) {
            auto h = _waiters.front();
            _waiters.pop();
            h.resume();
        }
    }

private:
    Callback _callback;                         ///< 用户任务处理回调
    std::queue<T> _queue;                       ///< 任务队列
    std::queue<std::coroutine_handle<>> _waiters; ///< 等待中的协程
    mutable std::mutex _mutex;                  ///< 队列互斥锁
    std::condition_variable _cv;                ///< 条件变量用于唤醒协程
    bool _running;                              ///< 是否处于运行状态
    int _coroutineCount;                        ///< 协程数量
    std::vector<ConsumerTask> _coroutines;      ///< 协程对象集合
    std::thread _worker;                        ///< 调度线程
};
