#pragma once

#include <queue>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <condition_variable>

/**
 * @class ThreadConsumer
 * @brief 基于线程的多线程任务消费者
 * @details
 * ThreadConsumer 使用固定数量的工作线程消费任务队列：
 * - 生产者通过 AddTask() 提交任务
 * - 消费者线程阻塞等待任务
 * - Stop() 可选择等待队列任务处理完成
 *
 * 适用于 CPU 密集或不支持协程的任务模型。
 *
 * @tparam T 任务类型
 * @author BUG
 * @date 2025-12-25
 */
template<typename T>
class ThreadConsumer{
public:
    /// 任务处理回调类型
    using Callback = std::function<void(T)>;

    /**
     * @brief 构造线程消费者
     * @details 初始化内部状态，但不创建线程
     *
     * @param func 用户提供的任务处理回调函数
     * @param threadCount 工作线程数量
     * @author BUG
     * @date 2025-12-25
     */
    ThreadConsumer(Callback func, int threadCount = 1)
        : _callback(std::move(func)), _running(false)
    {
        _threads.resize(threadCount);
    }

    /**
     * @brief 析构函数
     * @details 调用 Stop(true)，确保所有线程退出
     *
     * @author BUG
     * @date 2025-12-25
     */
    ~ThreadConsumer(){
        Stop(true);
    }

    /**
     * @brief 启动工作线程
     * @details 创建 threadCount 个线程执行 ThreadFunc
     * 重复调用无效
     *
     * @thread_safety 非线程安全，应在初始化阶段调用
     * @author BUG
     * @date 2025-12-25
     */
    void Start(){
        if(_running) return;

        _running = true;
        for(auto& t : _threads)
            t = std::thread(&ThreadConsumer::ThreadFunc, this);
    }

    /**
     * @brief 停止线程消费者
     * @details
     * 设置运行状态为 false，并唤醒所有线程。
     * 可选择等待任务队列处理完后再退出。
     *
     * @param wait_all_tasks true 等待队列任务完成，false 清空队列立即退出
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    void Stop(bool wait_all_tasks = false){
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            _running = false;
        }

        _cv.notify_all();

        if(wait_all_tasks){
            for(auto& t : _threads)
                if(t.joinable())
                    t.join();
        } else {
            {
                std::lock_guard<std::mutex> lock(_queue_mutex);
                std::queue<T> empty;
                std::swap(_task_queue, empty);
            }
            for(auto& t : _threads)
                if(t.joinable())
                    t.join();
        }
    }

    /**
     * @brief 添加任务（拷贝）
     * @details 将任务拷贝入队列，唤醒一个等待线程
     *
     * @param task 要处理的任务
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    void AddTask(const T& task){
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            if(!_running) return;
            _task_queue.push(task);
        }
        _cv.notify_one();
    }

    /**
     * @brief 添加任务（移动）
     * @details 将任务移动入队列，唤醒一个等待线程
     *
     * @param task 要处理的任务
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    void AddTask(T&& task){
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            if(!_running) return;
            _task_queue.push(std::move(task));
        }
        _cv.notify_one();
    }

    /**
     * @brief 获取当前任务队列大小
     * @return 队列中未处理任务数量
     * @thread_safety 线程安全
     * @author BUG
     * @date 2025-12-25
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _task_queue.size();
    }

private:
    /**
     * @brief 工作线程主函数
     * @details
     * - 等待任务或停止信号
     * - 取出任务执行回调
     * - Stop() 后在队列空时退出
     * @author BUG
     * @date 2025-12-25
     */
    void ThreadFunc(){
        while(true){
            T task;

            {
                std::unique_lock<std::mutex> lock(_queue_mutex);
                _cv.wait(lock, [&]{ return !_task_queue.empty() || !_running; });

                if(!_running && _task_queue.empty())
                    return;

                task = std::move(_task_queue.front());
                _task_queue.pop();
            }

            _callback(std::move(task));
        }
    }

private:
    bool _running;                     ///< 是否处于运行状态
    std::vector<std::thread> _threads; ///< 工作线程集合
    std::condition_variable _cv;       ///< 条件变量，用于唤醒线程
    Callback _callback;                ///< 用户任务处理回调
    std::queue<T> _task_queue;         ///< 等待处理的任务队列
    mutable std::mutex _queue_mutex;   ///< 保护任务队列的互斥锁
};
