#pragma once
#include <deque>
#include <coroutine>
#include <semaphore>
#include <optional>
#include <mutex>

template <typename T>
class AsyncPool;

template <typename T>
class AsyncPoolAwaiter {
public:
    AsyncPoolAwaiter(AsyncPool<T>& pool) : _pool(pool) {}
    ~AsyncPoolAwaiter() = default;

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) noexcept {
        _pool.enqueue_waiter(h);
    }

    T await_resume() noexcept { return std::move(_value.value()); }

private:
    AsyncPool<T>& _pool;
    std::optional<T> _value;   // 用 optional 避免未初始化
    friend class AsyncPool<T>;
};

template <typename T>
class AsyncPool {
public:
    AsyncPool(int max_size = 10, int init_size = 1)
        : _max_size(max_size), _size(0), _semaphore(0) 
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (int i = 0; i < init_size && _size < _max_size; ++i) {
            T obj = Create();
            if (Effective(obj)) {
                _pool.push_back(std::move(obj));
                _size++;
                _semaphore.release();
            }
        }
    }

    ~AsyncPool() {
        Clear();
    }

    // 协程获取对象
    AsyncPoolAwaiter<T> Get() {
        AsyncPoolAwaiter<T> awaiter(*this);

        std::lock_guard<std::mutex> lock(_mutex);

        // 如果池中有可用对象，直接返回
        if (!_pool.empty()) {
            awaiter._value = std::move(_pool.front());
            _pool.pop_front();
        } 
        // 否则尝试创建新对象
        else if (_size < _max_size) {
            T obj = Create();
            if (Effective(obj)) {
                awaiter._value = std::move(obj);
                _size++;
            }
        } 
        // 如果没有对象可用，也不能创建，则挂起协程
        // await_suspend 会自动处理
        return awaiter;
    }

    // 归还对象
    void Put(T item) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (Effective(item)) {
            _pool.push_back(std::move(item));
            _semaphore.release();
            resume_one();
        } else {
            // 如果对象失效，销毁并减少计数
            Destroy(item);
            _size--;
        }
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& item : _pool) Destroy(item);
        _pool.clear();
        _size = 0;
    }

protected:
    virtual T Create() = 0;
    virtual bool Effective(T item) = 0;
    virtual void Destroy(T item) = 0;

private:
    void enqueue_waiter(std::coroutine_handle<> h) {
        std::lock_guard<std::mutex> lock(_mutex);
        _waiters.push_back(h);
    }

    void resume_one() {
        if (!_waiters.empty()) {
            auto h = _waiters.front();
            _waiters.pop_front();
            h.resume();
        }
    }

private:
    int _size;
    int _max_size;
    std::deque<T> _pool;
    std::deque<std::coroutine_handle<>> _waiters;
    std::counting_semaphore<> _semaphore;
    std::mutex _mutex;

    friend class AsyncPoolAwaiter<T>;
};
