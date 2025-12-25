#pragma once

#include <Executor/ResultTask.h>
#include <Executor/ThreadExecutor.h>

/**
 * @class ThreadExecutorResult
 * @brief 基于 ThreadExecutor 的返回结果提交语义
 *
 * @tparam R 返回值类型
 *
 * @author BUG
 */
template<typename R>
class ThreadExecutorResult {
public:
    explicit ThreadExecutorResult(ThreadExecutor& exec)
        : _exec(exec) {}

    R Submit(std::function<R()> fn) {
        ResultTask<R> task(std::move(fn));
        _exec.Add(std::ref(task));
        return task.Get();
    }

private:
    ThreadExecutor& _exec;
};
