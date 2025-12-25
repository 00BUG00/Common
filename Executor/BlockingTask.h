#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>

/**
 * @class BlockingTask
 * @brief 可阻塞等待完成的任务封装
 *
 * @details
 * - 将一个 void() 任务包装为「可等待完成」
 * - 由调用线程阻塞等待
 * - Runtime 只负责执行 operator()
 *
 * @author BUG
 */
class BlockingTask {
public:
    explicit BlockingTask(std::function<void()> fn)
        : _fn(std::move(fn)) {}

    /// Runtime 执行入口
    void operator()() {
        _fn();
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _done = true;
        }
        _cv.notify_one();
    }

    /// 提交线程阻塞等待完成
    void Wait() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [&] { return _done; });
    }

private:
    std::function<void()> _fn;
    bool _done{false};
    std::mutex _mutex;
    std::condition_variable _cv;
};
