#pragma once

#include <map>
#include <list>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>

#ifdef JSON_CPP
#include <jsoncpp/json/json.h>
#endif

// 日志类型
enum class LOG_TYPE {
    INFO,
    ERROR,
    WARN,
    DEBUG,
};

// 宏定义
#ifndef LOGI
#define LOGI() Log(LOG_TYPE::INFO, __FILE__, __FUNCTION__, __LINE__)
#endif
#ifndef LOGE
#define LOGE() Log(LOG_TYPE::ERROR, __FILE__, __FUNCTION__, __LINE__)
#endif
#ifndef LOGW
#define LOGW() Log(LOG_TYPE::WARN, __FILE__, __FUNCTION__, __LINE__)
#endif
#ifndef LOGD
#define LOGD() Log(LOG_TYPE::DEBUG, __FILE__, __FUNCTION__, __LINE__)
#endif

class Log {
public:
    Log(LOG_TYPE type, const std::string& file, const char* function, int line) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &now_c);
#else
        localtime_r(&now_c, &tm);
#endif
        _content << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        switch(type){
            case LOG_TYPE::INFO:  _content << " INFO "; break;
            case LOG_TYPE::WARN:  _content << " WARN "; break;
            case LOG_TYPE::ERROR: _content << " ERROR "; break;
            case LOG_TYPE::DEBUG: _content << " DEBUG "; break;
        }
        _content << file.substr(file.find_last_of('/') + 1) << "[" << line << "][" << function << "] ";
    }

    template <typename T>
    Log& operator<<(const T& data){
        _content << data << " ";
        return *this;
    }

    template <typename T>
    Log& operator<<(const std::vector<T>& container){
        for (size_t i = 0; i < container.size(); ++i) {
            if (i != 0) _content << ",";
            _content << container[i];
        }
        _content << " ";
        return *this;
    }

    template <typename K, typename V>
    Log& operator<<(const std::map<K,V>& container){
        _content << "MAP:{";
        size_t i = 0;
        for (const auto& item : container) {
            _content << "[" << item.first << "," << item.second << "]";
            if (++i != container.size()) _content << ", ";
        }
        _content << "} ";
        return *this;
    }

#ifdef JSON_CPP
    Log& operator<<(const Json::Value& json){
        Json::StreamWriterBuilder writer_builder;
        writer_builder["indentation"] = "";
        writer_builder["enableYAMLCompatibility"] = true;
        writer_builder["emitUTF8"] = true;
        std::ostringstream oss;
        std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
        writer->write(json, &oss);
        _content << oss.str();
        return *this;
    }
#endif

#ifdef LOG_THREAD
    ~Log() {
        AddLog(_content.str());
    }

    // 异步日志线程
    static void LogThread() {
        while (true) {
            std::list<std::string> logs_to_write;

            // 获取锁并等待
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _condition_variable.wait(lock, []{
                    return !_content_list.empty() || !_thread_run_flag;
                });

                // 退出条件
                if (!_thread_run_flag && _content_list.empty())
                    break;

                logs_to_write.swap(_content_list);
            } // 锁释放

            // 写日志（不持锁）
            for (auto& log_str : logs_to_write) {
                if (_log_writer_func)
                    _log_writer_func(log_str);
                else
                    std::cout << log_str << std::endl;
            }
        }
    }

    static void StartLogThread() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_thread.joinable()) {
            _thread_run_flag = true;
            _thread = std::thread(LogThread);
        }
    }

    static void StopLogThread() {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _thread_run_flag = false;
        }
        _condition_variable.notify_all();
        if (_thread.joinable())
            _thread.join();
    }

    static void AddLog(const std::string& content) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _content_list.push_back(content);
        }
        _condition_variable.notify_one();
    }

#else
    ~Log() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_log_writer_func)
            _log_writer_func(_content.str());
        else
            std::cout << _content.str() << std::endl;
    }
#endif

    static inline void SetLogWriterFunc(std::function<void(const std::string&)> func){
        _log_writer_func = func;
    }

private:
    std::ostringstream _content;

    static inline std::mutex _mutex;
    static inline std::function<void(const std::string&)> _log_writer_func = nullptr;

#ifdef LOG_THREAD
    static inline std::list<std::string> _content_list;
    static inline std::thread _thread;
    static inline bool _thread_run_flag = false;
    static inline std::condition_variable _condition_variable;
#endif
};
