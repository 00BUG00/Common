#pragma once

#include <coroutine>
#include <vector>
#include <atomic>
#include <thread>

/**
 * @class CoroutineExecutor
 * @brief 基于协程的 Runtime 执行器
 *
 * ============================================================
 * 一、角色定位（Role）
 * ============================================================
 *
 * CoroutineExecutor 是 Runtime 层的一种实现。
 *
 * 它的目标不是“更快”，而是：
 *   - 更低上下文切换成本
 *   - 可控的协作式调度
 *
 * ============================================================
 * 二、执行模型（Execution Model）
 * ============================================================
 *
 * - 单 OS 线程
 * - 多个消费者协程
 * - 协程主动让出执行权（co_await）
 *
 * ============================================================
 * 三、与 ThreadExecutor 的核心区别
 * ============================================================
 *
 * ThreadExecutor：
 *   - 抢占式调度（OS）
 *   - 阻塞等待
 *
 * CoroutineExecutor：
 *   - 协作式调度（用户态）
 *   - 无阻塞等待
 *
 * ============================================================
 * 四、不变量（Invariants）
 * ============================================================
 *
 * 1. 不创建多个 OS 线程
 * 2. 不使用 condition_variable
 * 3. 不阻塞
 * 4. 所有切换点必须显式 co_await
 *
 * ============================================================
 * 五、适用场景（Use Case）
 * ============================================================
 *
 * - IO 密集型
 * - Actor / 消息驱动模型
 * - 游戏主循环
 *
 * ============================================================
 */
template<typename Task>
class CoroutineExecutor {
public:
    /**
     * @brief 构造函数
     *
     * @param executor Executor 层
     * @param coroutineCount 消费协程数量
     *
     * @note
     * - 不启动协程
     * - 仅做结构初始化
     */
    CoroutineExecutor(LockFreeExecutor<Task>& executor, size_t coroutineCount)
        : _executor(executor),
          _running(false),
          _coroutineCount(coroutineCount) {}

    /**
     * @brief 析构函数
     *
     * @details
     * - 确保调度线程退出
     * - 不允许悬挂协程
     */
    ~CoroutineExecutor() {
        Stop();
    }

    /**
     * @brief 启动 Runtime
     *
     * ========================================================
     * 执行步骤：
     * ========================================================
     *
     * 1. 创建 consumer 协程
     * 2. 创建调度线程
     * 3. 调度线程轮询 resume 协程
     */
    void Start() {
        if (_running.exchange(true))
            return;

        for (size_t i = 0; i < _coroutineCount; ++i) {
            _tasks.emplace_back(ConsumeLoop());
        }

        _worker = std::thread(&CoroutineExecutor::SchedulerLoop, this);
    }

    /**
     * @brief 停止 Runtime
     *
     * @details
     * - 设置运行标志
     * - 等待调度线程退出
     *
     * @note
     * 不保证所有任务执行完成
     */
    void Stop() {
        if (!_running.exchange(false))
            return;

        if (_worker.joinable())
            _worker.join();
    }

    /**
     * @brief 提交任务
     *
     * @details
     * - 非阻塞
     * - 仅转发到 Executor
     */
    bool Submit(const Task& task) {
        return _executor.Add(task);
    }

private:
    /**
     * @brief 协程句柄封装
     *
     * @note
     * 用于保存 coroutine_handle
     */
    struct TaskHandle {
        struct promise_type {
            TaskHandle get_return_object() {
                return TaskHandle{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }
            std::suspend_always initial_suspend() noexcept { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void return_void() noexcept {}
            void unhandled_exception() { std::terminate(); }
        };

        std::coroutine_handle<promise_type> h;
    };

    /**
     * @brief 消费者协程主循环
     *
     * ========================================================
     * 执行语义：
     * ========================================================
     *
     * while (running):
     *   if TryPop 成功:
     *       执行任务
     *   co_await 挂起
     *
     * ========================================================
     * 关键点：
     * ========================================================
     *
     * - 没有阻塞
     * - 没有等待
     * - 所有切换点可控
     */
    TaskHandle ConsumeLoop() {
        while (_running.load()) {
            Task task;
            if (_executor.TryPop(task)) {
                task();
            }
            co_await std::suspend_always{};
        }
    }

    /**
     * @brief 协程调度循环
     *
     * @details
     * - 在单线程中轮询 resume 所有协程
     * - 类似 cooperative scheduler
     */
    void SchedulerLoop() {
        while (_running.load()) {
            for (auto& t : _tasks) {
                if (t.h && !t.h.done())
                    t.h.resume();
            }
            std::this_thread::yield();
        }
    }

private:
    LockFreeExecutor<Task>& _executor; ///< Executor 层
    std::atomic<bool> _running;        ///< Runtime 状态

    size_t _coroutineCount;            ///< 协程数量
    std::vector<TaskHandle> _tasks;    ///< 消费协程集合

    std::thread _worker;               ///< 调度线程
};
