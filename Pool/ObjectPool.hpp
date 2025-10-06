#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>

template <typename T>
class ObjectPool
{
public:
    ObjectPool(int max_size = 100,int size = 10){
        _max_size = max_size;
        for(int i = 0; i < size; ){
            T item = Create();
            if(Effective(item)){
                _pool.push_back(item);
                _size++;
                i++;
            }else{
                Destroy(item);
            }
        }
    }

    ~ObjectPool(){
        Clear();
    }

    T Get(){
        while(true){
            std::unique_lock<std::mutex> lock(_mutex);
            /*如果队列中有对象，则拿取第一个对象，如果对象有效，则返回，否则销毁并继续循环*/
            if(!_pool.empty()){
                T item = _pool.front();
                _pool.pop_front();
                if(Effective(item)){
                    return item;
                }else{
                    _size--;
                    Destroy(item);
                    continue;
                }
            }
            /*如果队列中没有对象，则创建一个对象，如果对象有效，则返回，否则销毁并继续循环*/
            if(_size < _max_size){
                T item = Create();
                if(Effective(item)){
                    _size++;
                    return item;
                }else{
                    Destroy(item);
                    continue;
                }
            }
            /*如果队列中没有对象，且对象数量小于最大数量，则挂起线程等待有对象被归还*/
            _condition.wait(lock,[this](){return !_pool.empty() || _size < _max_size;});
        }
    }

    void Clear(){
        std::lock_guard<std::mutex> lock(_mutex);
        for(auto item : _pool){
            Destroy(item);
        }
        _pool.clear();
        _size = 0;
    }
    
    void Release(T item){
        std::lock_guard<std::mutex> lock(_mutex);
        _pool.push_back(item);
        _condition.notify_one();
    }
    
    virtual T Create() noexcept = 0;
    virtual bool Effective(T item) noexcept = 0;
    virtual void Destroy(T item) noexcept = 0;

private:
    int _size;
    int _max_size;
    std::deque<T> _pool;
    std::mutex _mutex;
    std::condition_variable _condition;
};