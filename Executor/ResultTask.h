#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>

/**
 * @class ResultTask
 * @brief 带返回值的任务封装
 *
 * @tparam R 返回值类型
 *
 * @details
 * - 将 R() 任务包装为「可等待结果」
 * - Get() 会阻塞直到执行完成
 * - Runtime 不感知返回值
 *
 * @author BUG
 */
template<typename R>
class ResultTask {
public:
    explicit ResultTask(std::function<R()> fn)
        : _fn(std::move(fn)) {}

    /// Runtime 执行入口
    void operator()() {
        R r = _fn();
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _result = std::move(r);
            _done = true;
        }
        _cv.notify_one();
    }

    /// 提交线程阻塞获取结果
    R Get() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [&] { return _done; });
        return _result;
    }

private:
    std::function<R()> _fn;
    R _result{};
    bool _done{false};
    std::mutex _mutex;
    std::condition_variable _cv;
};
