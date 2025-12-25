#pragma once

#include <Executor/BlockingTask.h>
#include <Executor/ThreadExecutor.h>

/**
 * @class ThreadExecutorBlocking
 * @brief 基于 ThreadExecutor 的阻塞提交语义
 *
 * @details
 * - Submit() 会阻塞直到任务执行完成
 * - 不修改 ThreadExecutor 本身
 *
 * @author BUG
 */
class ThreadExecutorBlocking {
public:
    explicit ThreadExecutorBlocking(ThreadExecutor& exec)
        : _exec(exec) {}

    void Submit(std::function<void()> fn) {
        BlockingTask task(std::move(fn));
        _exec.Add(std::ref(task));
        task.Wait();
    }

private:
    ThreadExecutor& _exec;
};
