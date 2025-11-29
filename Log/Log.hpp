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
#include <type_traits>
#include <iterator>
#include <unordered_map>
#include <string_view>
#include <utility>

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

typedef struct{
    LOG_TYPE type;
    std::string file;
    int line;
    std::string function;
    std::tm local_time;
    std::string content;
}LogData;

// 宏定义

#ifndef LOGI
    #define LOGI() Log(LOG_TYPE::INFO, __FILE__, __func__, __LINE__)
#endif

#ifndef LOGE
    #define LOGE() Log(LOG_TYPE::ERROR, __FILE__, __func__, __LINE__)
#endif

#ifndef LOGW
    #define LOGW() Log(LOG_TYPE::WARN, __FILE__, __func__, __LINE__)
#endif

#ifndef LOGD
    #define LOGD() Log(LOG_TYPE::DEBUG, __FILE__, __func__, __LINE__)
#endif

// 是否可迭代（有 begin/end 的类型）
template <typename T>
class is_iterable {
private:
    template <typename U>
    static auto test(int) -> decltype(
        std::begin(std::declval<U>()),
        std::end(std::declval<U>()),
        std::true_type()
    );

    template <typename>
    static std::false_type test(...);
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// 判断是否为 key-value 容器（value_type 有 first & second）
template<typename T>
struct is_kv_container {
private:
    template<typename U>
    static auto test(int) 
        -> decltype(
            std::declval<typename U::value_type>().first,
            std::declval<typename U::value_type>().second,
            std::true_type{}
        );

    template<typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

// 判断是否为 string-like（可以转换为 string_view）
// 这样可以屏蔽 std::string / std::string_view / const char* 等
template <typename T>
constexpr bool is_string_like_v = std::is_convertible_v<T, std::string_view>;


// ------------------ Log class ------------------

class Log {
public:
    Log(LOG_TYPE type, const std::string& file, const char* function, int line){
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        // 转换为时间戳
        auto now_c = std::chrono::system_clock::to_time_t(now);
        // 转换为本地时间
        std::tm* local_time = std::localtime(&now_c);
        _local_time = *local_time;
        _type = type;
        _file = file;
        _line = line;
        _log_function = function;
    }

    static std::string ToString(LogData LogData) {
        std::ostringstream content;
        content << std::put_time(&LogData.local_time, "%Y-%m-%d %H:%M:%S");
        content << " ";
        switch(LogData.type){
            case LOG_TYPE::INFO:  content << " I "; break;
            case LOG_TYPE::WARN:  content << " W "; break;
            case LOG_TYPE::ERROR: content << " E "; break;
            case LOG_TYPE::DEBUG: content << " D "; break;
        }
        content << LogData.file;
        content << "[" << LogData.line << "][" << LogData.function << "] ";
        return content.str() + LogData.content;
    }
    
    // 普通单值输出（保留）
    template <typename T>
    std::enable_if_t<
        !is_iterable<T>::value || 
        std::is_same_v<T, std::string> ||
        std::is_same_v<T, std::string_view>,
    Log&>
    operator<<(const T& data){
        _content << data << " ";
        return *this;
    }

    // ------------------
    // 通用可迭代容器重载（排除 string-like 与 KV 容器）
    // ------------------
    template <typename T>
    std::enable_if_t<
        is_iterable<T>::value &&
        !is_string_like_v<T> &&
        !is_kv_container<T>::value,
    Log&>
    operator<<(const T& container)
    {
        _content << "{";
        bool first = true;
        for (const auto& item : container) {
            if (!first) _content << ", ";
                _content << item;
            first = false;
        }
        _content << "} ";
        return *this;
    }

    // ------------------
    // 通用 KV 容器重载（map / unordered_map / multimap / custom KV）
    // ------------------
    template<typename T>
    std::enable_if_t<is_kv_container<T>::value, Log&>
    operator<<(const T& container)
    {
        _content << "MAP:{";
        bool first = true;
        for (const auto& item : container) {
            if (!first) _content << ", ";
            _content << "[" << item.first << "," << item.second << "]";
            first = false;
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

    ~Log() {
        if (_log_writer_func)
            _log_writer_func(ToLogInfo());
        else
            std::cout << ToString(ToLogInfo()) << std::endl;
    }


    static inline void SetLogWriterFunc(std::function<void(LogData)> func){
        _log_writer_func = func;
    }

    LogData ToLogInfo() {
        LogData LogData;
        LogData.type = _type;
        LogData.file = _file;
        LogData.line = _line;
        LogData.function = _log_function;
        LogData.local_time = _local_time;
        LogData.content = _content.str();
        return LogData;
    }

private:
    LOG_TYPE _type;
    std::string _file;
    int _line;
    std::string _log_function;
    std::tm _local_time;
    std::ostringstream _content;
    static inline std::function<void(const LogData)> _log_writer_func = nullptr;
};
