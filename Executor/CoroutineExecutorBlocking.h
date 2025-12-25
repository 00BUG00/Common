#pragma once

#include <Executor/BlockingTask.h>
#include <Executor/CoroutineExecutor.h>

/**
 * @class CoroutineExecutorBlocking
 * @brief 基于 CoroutineExecutor 的阻塞提交语义
 *
 * @details
 * - 调用线程阻塞
 * - Runtime 内部仍是单线程多协程
 *
 * @author BUG
 */
class CoroutineExecutorBlocking {
public:
    explicit CoroutineExecutorBlocking(CoroutineExecutor& exec)
        : _exec(exec) {}

    void Submit(std::function<void()> fn) {
        BlockingTask task(std::move(fn));
        _exec.Add(std::ref(task));
        task.Wait();
    }

private:
    CoroutineExecutor& _exec;
};
