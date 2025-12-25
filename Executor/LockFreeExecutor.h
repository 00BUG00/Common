#pragma once

#include <cstddef>
#include <functional>
#include <Containers/RingQueue.h>

/**
 * @class LockFreeExecutor
 * @brief 无锁 Executor（仅负责任务存取）
 *
 * @details
 * 【设计定位】
 * LockFreeExecutor 是整个 Runtime 体系中的“最底层执行容器”，
 * 只负责：
 *   - 并发安全地存储任务
 *   - 并发安全地弹出任务
 *
 * 它【不负责】：
 *   - 等待（wait / sleep）
 *   - 唤醒（notify）
 *   - 执行任务
 *   - 调度策略
 *
 * 【并发模型】
 * - 基于无锁 RingQueue
 * - 所有接口均为 Try 语义（非阻塞）
 * - 同时支持多线程 / 多协程访问
 *
 * 【重要不变量】
 * 1. 所有函数均为 non-blocking
 * 2. 不调用 yield / sleep / condition_variable
 * 3. 不持有任何执行上下文（线程 / 协程）
 *
 * 这些不变量使其可以：
 * - 被 ThreadExecutor 使用
 * - 被 CoroutineExecutor 使用
 * - 被未来的 IO / Actor Runtime 使用
 *
 * @tparam Task 任务类型
 *         - 常见为 std::function<void()>
 *         - 也可以是强类型任务结构体
 *
 * @author BUG
 * @date 2025-12-22
 */
template<typename Task>
class LockFreeExecutor {
public:
    /**
     * @brief 构造函数
     *
     * @param capacity 队列容量
     *
     * @note
     * - capacity 是硬容量
     * - 满时 Add() 会失败，不会阻塞
     */
    explicit LockFreeExecutor(size_t capacity)
        : _queue(capacity) {}

    /**
     * @brief 尝试添加一个任务
     *
     * @details
     * - 非阻塞
     * - 线程 / 协程安全
     * - 不保证一定成功
     *
     * @param task 要添加的任务
     *
     * @return
     * - true  添加成功
     * - false 队列 Full 或发生并发竞争
     *
     * @note
     * 失败后由上层 Runtime 决定：
     * - 重试
     * - yield
     * - sleep
     * - 丢弃
     */
    bool Add(const Task& task) {
        return _queue.TryPush(task) == RingQueueResult::Ok;
    }

    /**
     * @brief 尝试弹出一个任务
     *
     * @details
     * - 非阻塞
     * - 不等待任务到来
     *
     * @param out 用于接收任务
     *
     * @return
     * - true  成功弹出
     * - false 当前无任务或发生竞争
     */
    bool TryPop(Task& out) {
        return _queue.TryPop(out) == RingQueueResult::Ok;
    }

    /**
     * @brief 获取当前队列大小（近似值）
     *
     * @note
     * 仅用于监控 / 调试，不作为逻辑判断依据
     */
    size_t SizeApprox() const {
        return _queue.SizeApprox();
    }

private:
    RingQueue<Task> _queue; ///< 无锁任务队列
};
