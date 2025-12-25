#pragma once

#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <string>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <type_traits>
#include "JsonSerializable/TypeTraits.h"
#include "JsonSerializable/FieldMacros.h"

/**
 * @brief JSON 序列化基类
 * @details 提供将对象序列化为 JSON 数据的能力，支持基本类型、STL 容器、可选类型以及继承自 JsonSerializer 的自定义类型。
 * 使用方法：
 * 1. 继承 JsonSerializer。
 * 2. 重载 to_json() 或使用宏自动生成序列化函数。
 * 3. 调用 to_json() 或 to_json_string() 获取 JSON 对象或字符串。
 */
class JsonSerializer {
public:
    virtual ~JsonSerializer() = default;

    /**
     * @brief 将对象序列化为 Json::Value
     * @return Json::Value 对象
     */
    virtual Json::Value to_json() const { return Json::Value(); }

    // ========================= 可选类型序列化 =========================

    /**
     * @brief 将多个可选字段序列化到 JSON 对象
     * @tparam T 第一个字段类型
     * @tparam Args 剩余字段类型
     * @param j JSON 对象引用
     * @param name 当前字段名称
     * @param value 当前字段的可选值指针
     * @param args 其他字段参数包
     */
    template<typename T, typename... Args>
    void to_json(Json::Value& j, std::string name, const std::optional<T>* value, Args... args) const {
        to_json(j, name, value);
        to_json(j, args...);
    }

    /**
     * @brief 将单个可选字段序列化到 JSON 对象
     * @tparam T 字段类型
     * @param j JSON 对象引用
     * @param name 字段名称
     * @param value 字段的可选值指针
     */
    template<typename T>
    void to_json(Json::Value& j, std::string name, const std::optional<T>* value) const {
        if (value == nullptr || !value->has_value()) {
            return;
        }
        j[name] = to_json_value(value->value());
    }

    // ========================= 辅助方法 =========================

    /**
     * @brief 将任意支持类型转换为 Json::Value
     * @tparam T 类型
     * @param value 待转换的值
     * @return Json::Value
     * @details 支持：
     *  - 基本类型
     *  - 序列容器 (vector, list, deque)
     *  - 集合容器 (set, unordered_set)
     *  - 关联容器 (map, unordered_map)
     *  - 自定义 JsonSerializer 类型
     */
    template<typename T>
    Json::Value to_json_value(const T& value) const {
        if constexpr (std::is_base_of_v<JsonSerializer, T>) {
            return value.to_json();
        } else if constexpr (is_sequence_container<T>::value || is_set_container<T>::value) {
            Json::Value arr(Json::arrayValue);
            for (const auto& e : value) arr.append(to_json_value(e));
            return arr;
        } else if constexpr (is_associative_container<T>::value) {
            Json::Value obj(Json::objectValue);
            for (const auto& pair : value) {
                std::string key = to_string(pair.first);
                obj[key] = to_json_value(pair.second);
            }
            return obj;
        } else {
            return Json::Value(value);
        }
    }

    /**
     * @brief 将键值类型转换为字符串
     * @tparam K 键类型
     * @param key 待转换的键
     * @return 字符串
     */
    template<typename K>
    std::string to_string(const K& key) const {
        if constexpr (std::is_arithmetic_v<K>) {
            return std::to_string(key);
        } else if constexpr (std::is_same_v<K, std::string>) {
            return key;
        } else {
            return std::to_string(key);
        }
    }

    /**
     * @brief 将可选对象序列化为 Json::Value
     * @tparam T 类型
     * @param value 可选对象
     * @return Json::Value 对象
     */
    template<typename T>
    Json::Value to_json(std::optional<T> value) {
        if (!value.has_value()) return Json::Value();
        
        if constexpr (std::is_base_of_v<JsonSerializer, T>) {
            return value.value().to_json();
        } else if constexpr (is_sequence_container<T>::value || is_set_container<T>::value) {
            Json::Value arr(Json::arrayValue);
            for (const auto& e : value.value()) arr.append(to_json_value(e));
            return arr;
        } else if constexpr (is_associative_container<T>::value) {
            Json::Value obj(Json::objectValue);
            for (const auto& pair : value.value()) {
                std::string key = to_string(pair.first);
                obj[key] = to_json_value(pair.second);
            }
            return obj;
        } else {
            return Json::Value(value.value());
        }
    }
};

// ========================= 宏定义 =========================

/**
 * @brief 自动生成 JSON 序列化函数宏
 * @param BASE 父类名
 * @param ... 要序列化的字段名称
 * @details 在类内部使用该宏即可自动生成 to_json() 函数
 */
#ifndef JSON_SERIALIZE
#define JSON_SERIALIZE(BASE, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j; BASE::to_json(j, __VA_ARGS__); return j; \
    }
#endif

/**
 * @brief 自动生成 JSON 序列化函数宏（包含父类字段）
 * @param BASE 父类名
 * @param ... 要序列化的字段名称
 */
#ifndef JSON_SERIALIZE_WITH_PARENT
#define JSON_SERIALIZE_WITH_PARENT(BASE, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j; BASE::to_json(j, __VA_ARGS__); return j; \
    }
#endif

/**
 * @brief 自动生成 JSON 序列化函数宏（继承父类字段并序列化自身字段）
 * @param PARENT_CLASS 父类名
 * @param ... 要序列化的字段名称
 */
#ifndef JSON_SERIALIZE_INHERIT
#define JSON_SERIALIZE_INHERIT(PARENT_CLASS, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j = PARENT_CLASS::to_json(); \
        JsonSerializer::to_json(j, __VA_ARGS__); \
        return j; \
    }
#endif

/**
 * @brief 为类提供便捷 JSON 转字符串方法宏
 * @param CLASS_NAME 类名
 * @details 提供：
 *  - 成员对象 to_json_string()
 *  - 对象数组 std::vector<CLASS_NAME> 的 to_json_string()
 */
#ifndef TO_JSON_METHODS
#define TO_JSON_METHODS(CLASS_NAME) \
public: \
    std::string to_json_string() const { \
        Json::Value j = to_json(); \
        Json::StreamWriterBuilder builder; \
        return Json::writeString(builder, j); \
    } \
    static std::string to_json_string(const std::vector<CLASS_NAME>& objects) { \
        Json::Value arr(Json::arrayValue); \
        for (const auto& obj : objects) { \
            arr.append(obj.to_json()); \
        } \
        Json::StreamWriterBuilder builder; \
        return Json::writeString(builder, arr); \
    }
#endif
