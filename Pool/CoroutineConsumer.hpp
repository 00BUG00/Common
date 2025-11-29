#pragma once

#include <coroutine>
#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

template<typename T>
class CoroutineConsumer {
public:
    using Callback = std::function<void(const T&)>;

    CoroutineConsumer(Callback func, int coroutineCount = 1)
        : _callback(func), _running(false), _coroutineCount(coroutineCount)
    {}

    ~CoroutineConsumer() {
        Stop();
    }

    void Start() {
        if (_running)
            return;

        _running = true;

        for (int i = 0; i < _coroutineCount; i++) {
            _coroutines.push_back(ConsumeCoroutine());
        }
        _worker = std::thread(&CoroutineConsumer::EventLoop, this);
    }

    void Stop() {
        if (!_running)
            return;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _running = false;
        }

        _cv.notify_all();

        if (_worker.joinable())
            _worker.join();
    }

    void AddTask(const T& task) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_running) return;
            _queue.push(task);
        }
        _cv.notify_one();
    }

private:

    struct TaskAwaiter {
        CoroutineConsumer* self;
        T value;

        bool await_ready() {
            std::lock_guard<std::mutex> lock(self->_mutex);
            return !self->_queue.empty();
        }

        void await_suspend(std::coroutine_handle<> h) {
            self->_waiters.push(h);
        }

        T await_resume() {
            std::lock_guard<std::mutex> lock(self->_mutex);
            value = self->_queue.front();
            self->_queue.pop();
            return value;
        }
    };

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

        std::coroutine_handle<promise_type> h;
    };

    ConsumerTask ConsumeCoroutine() {
        while (true) {
            // 若 Stop 且无任务，该协程退出
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_running && _queue.empty())
                    co_return;
            }

            T task = co_await TaskAwaiter{this};
            _callback(task);
        }
    }

    void EventLoop() {
        while (true) {
            std::coroutine_handle<> waiter;

            {
                std::unique_lock<std::mutex> lock(_mutex);
                
                if (!_running && _queue.empty() && _waiters.empty())
                    break;

                _cv.wait(lock, [&] {
                    return !_queue.empty() || !_running;
                });

                if (_waiters.empty())
                    continue;

                // 唤醒一个等待协程
                waiter = _waiters.front();
                _waiters.pop();
            }

            // 恢复协程
            if (waiter)
                waiter.resume();
        }

        while (!_waiters.empty()) {
            auto h = _waiters.front();
            _waiters.pop();
            h.resume();
        }
    }

private:
    Callback _callback;
    std::queue<T> _queue;
    std::queue<std::coroutine_handle<>> _waiters;

    std::mutex _mutex;
    std::condition_variable _cv;

    bool _running;
    int _coroutineCount;

    std::vector<ConsumerTask> _coroutines;
    std::thread _worker;
};
