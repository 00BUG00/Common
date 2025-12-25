#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>

/**
 * @class ThreadExecutor
 * @brief 基于线程的 Runtime 执行器
 *
 * ============================================================
 * 一、角色定位（Role）
 * ============================================================
 *
 * ThreadExecutor 是 Runtime 层，而不是 Executor 层。
 *
 * 它的唯一职责是：
 *   - 决定「线程如何等待任务」
 *   - 决定「什么时候唤醒线程」
 *   - 决定「任务在哪个线程上执行」
 *
 * 它不参与：
 *   - 任务数据结构设计
 *   - 并发安全队列实现
 *   - 任务生命周期管理
 *
 * ============================================================
 * 二、协作关系（Collaboration）
 * ============================================================
 *
 * ThreadExecutor 依赖：
 *   - LockFreeExecutor<Task>
 *
 * 协作方式：
 *   - LockFreeExecutor 负责：
 *       * Add / TryPop（非阻塞）
 *   - ThreadExecutor 负责：
 *       * wait / notify
 *       * 线程调度
 *
 * ============================================================
 * 三、并发模型（Concurrency Model）
 * ============================================================
 *
 * - 多生产者：任意线程可 Submit
 * - 多消费者：多个工作线程并行执行
 *
 * - Executor 层：无锁 / Try 语义
 * - Runtime 层：允许阻塞（condition_variable）
 *
 * ============================================================
 * 四、不变量（Invariants）【非常重要】
 * ============================================================
 *
 * 1. ThreadExecutor 永远不会存储任务
 * 2. ThreadExecutor 永远不会关心队列容量
 * 3. ThreadExecutor 不实现重试、超时、批处理
 * 4. 所有等待策略只存在于 Runtime
 *
 * 破坏以上任一条，都会导致 Executor / Runtime 边界崩溃
 *
 * ============================================================
 * 五、生命周期（Lifecycle）
 * ============================================================
 *
 *   Start()  -> 创建线程并进入 WorkerLoop
 *   Stop()   -> 通知线程退出并 join
 *   ~ThreadExecutor() -> 强制 Stop
 *
 * ============================================================
 * 六、使用场景（Use Case）
 * ============================================================
 *
 * - CPU 任务并行执行
 * - 后台 Fire-and-Forget
 * - 与 Future / Promise 组合使用
 *
 * ============================================================
 */
template<typename Task>
class ThreadExecutor {
public:
    /**
     * @brief 构造函数
     *
     * @param executor 任务容器（Executor 层）
     * @param threadCount 工作线程数量
     *
     * @note
     * - executor 的生命周期必须长于 ThreadExecutor
     * - 不创建线程，仅做资源准备
     */
    ThreadExecutor(LockFreeExecutor<Task>& executor, size_t threadCount)
        : _executor(executor), _running(false) {
        _threads.resize(threadCount);
    }

    /**
     * @brief 析构函数
     *
     * @details
     * 析构即意味着 Runtime 生命周期结束：
     * - 必须保证所有线程安全退出
     * - 不允许后台悬挂线程
     */
    ~ThreadExecutor() {
        Stop();
    }

    /**
     * @brief 启动 Runtime
     *
     * @details
     * - 创建并启动工作线程
     * - 每个线程进入 WorkerLoop
     *
     * @note
     * - 非线程安全
     * - 只能在初始化阶段调用
     */
    void Start() {
        if (_running.exchange(true))
            return;

        for (auto& t : _threads) {
            t = std::thread(&ThreadExecutor::WorkerLoop, this);
        }
    }

    /**
     * @brief 停止 Runtime
     *
     * @details
     * - 设置运行状态为 false
     * - 唤醒所有等待线程
     * - join 等待线程退出
     *
     * @note
     * Stop 不保证任务全部执行完成
     */
    void Stop() {
        if (!_running.exchange(false))
            return;

        _cv.notify_all();

        for (auto& t : _threads) {
            if (t.joinable())
                t.join();
        }
    }

    /**
     * @brief 提交任务
     *
     * @details
     * - 线程安全
     * - 仅负责转发任务到 Executor
     * - 成功后负责唤醒一个线程
     *
     * @return
     * - true  提交成功
     * - false Executor 满或竞争失败
     */
    bool Submit(const Task& task) {
        bool ok = _executor.Add(task);
        if (ok) {
            _cv.notify_one();
        }
        return ok;
    }

private:
    /**
     * @brief 工作线程主循环
     *
     * ========================================================
     * 执行逻辑：
     * ========================================================
     *
     * while (running):
     *   if TryPop 成功:
     *       执行任务
     *   else:
     *       进入等待
     *
     * ========================================================
     * 设计说明：
     * ========================================================
     *
     * - TryPop 永远不阻塞
     * - 阻塞行为只发生在 condition_variable
     * - 这是 Runtime 层的核心逻辑
     */
    void WorkerLoop() {
        while (_running.load()) {
            Task task;

            if (_executor.TryPop(task)) {
                task();
            } else {
                std::unique_lock<std::mutex> lock(_waitMutex);
                _cv.wait_for(lock, std::chrono::milliseconds(1));
            }
        }
    }

private:
    LockFreeExecutor<Task>& _executor; ///< Executor 层（任务容器）
    std::atomic<bool> _running;        ///< Runtime 运行状态

    std::vector<std::thread> _threads; ///< 工作线程集合

    std::condition_variable _cv; ///< 线程等待/唤醒
    std::mutex _waitMutex;
};
