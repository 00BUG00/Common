#pragma once

#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

template<typename T>
class ThreadConsumer{
public:
    using Callback = std::function<void(const T&)>;

    ThreadConsumer(Callback func, int threadCount = 1)
        : _callback(func), _running(false)
    {
        _threads.resize(threadCount);
    }
    
    ~ThreadConsumer(){
        Stop();
    }

    void AddTask(const T& task){
        std::lock_guard<std::mutex> lock(_queue_mutex);
        if (!_running)
            return;

        _task_queue.push(task);
        _cv.notify_one();
    }

    void Start(){
        if(_running)
            return;

        _running = true;

        for (auto& t : _threads)
            t = std::thread(&ThreadConsumer::ThreadFunc, this);
    }

    void Stop(){
        if(!_running)
            return;

        _running = false;

        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            std::queue<T> empty;
            std::swap(_task_queue, empty); // 正确清空 queue
        }

        _cv.notify_all();

        for (auto& t : _threads)
            if(t.joinable())
                t.join();
    }

    void ThreadFunc(){
        while (true){
            T task;

            {
                std::unique_lock<std::mutex> lock(_queue_mutex);

                // 等待直到：有任务 或 running = false
                _cv.wait(lock, [&] {
                    return !_task_queue.empty() || !_running;
                });

                // 停止 且 队列空 → 退出线程
                if (!_running && _task_queue.empty())
                    return;

                // 取出任务
                task = _task_queue.front();
                _task_queue.pop();
            }

            // 锁外执行回调
            _callback(task);
        }
    }

private:
    bool _running;
    std::vector<std::thread> _threads;
    std::condition_variable _cv;
    Callback _callback;
    std::queue<T> _task_queue;
    std::mutex _queue_mutex;
};
