#pragma once

#include <Executor/ResultTask.h>
#include <Executor/CoroutineExecutor.h>

/**
 * @class CoroutineExecutorResult
 * @brief 基于 CoroutineExecutor 的返回结果提交语义
 *
 * @tparam R 返回值类型
 *
 * @author BUG
 */
template<typename R>
class CoroutineExecutorResult {
public:
    explicit CoroutineExecutorResult(CoroutineExecutor& exec)
        : _exec(exec) {}

    R Submit(std::function<R()> fn) {
        ResultTask<R> task(std::move(fn));
        _exec.Add(std::ref(task));
        return task.Get();
    }

private:
    CoroutineExecutor& _exec;
};
