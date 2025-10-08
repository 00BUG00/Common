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
 * JSON 序列化功能
 * 提供将对象序列化为 JSON 数据的能力
 */

// ------------------ 基础类 ------------------
class JsonSerializer {
public:
    virtual ~JsonSerializer() = default;
    virtual Json::Value to_json() const { return Json::Value(); }

    template<typename T, typename... Args>
    void to_json(Json::Value& j, std::string name, const std::optional<T>* value, Args... args) const {
        to_json(j, name, value);
        to_json(j, args...);
    }

    template<typename T>
    void to_json(Json::Value& j, std::string name, const std::optional<T>* value) const {
        if (value == nullptr || !value->has_value()) {
            return;
        }
        j[name] = to_json_value(value->value());
    }

    // 辅助方法：将不同类型的值转换为 Json::Value
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

    // 辅助方法：将键转换为字符串
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

// ------------------ 宏定义 ------------------

// JSON 序列化宏
#ifndef JSON_SERIALIZE
#define JSON_SERIALIZE(BASE, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j; BASE::to_json(j, __VA_ARGS__); return j; \
    }
#endif

// JSON 序列化宏（包含父类字段）
#ifndef JSON_SERIALIZE_WITH_PARENT
#define JSON_SERIALIZE_WITH_PARENT(BASE, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j; BASE::to_json(j, __VA_ARGS__); return j; \
    }
#endif

// JSON 序列化宏（自动处理父类字段）
#ifndef JSON_SERIALIZE_INHERIT
#define JSON_SERIALIZE_INHERIT(PARENT_CLASS, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j = PARENT_CLASS::to_json(); \
        JsonSerializer::to_json(j, __VA_ARGS__); \
        return j; \
    }
#endif

// 便捷的将对象转换为 JSON 的静态方法宏
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

// 使用示例：
/*
// 基础类示例
class User : public JsonSerializer {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)
    FIELD(std::vector<std::string>, tags)
    FIELD(std::map<std::string, std::string>, metadata)
    
    JSON_SERIALIZE(JsonSerializer, 
        FIELD_PAIR(id),
        FIELD_PAIR(name),
        FIELD_PAIR(email),
        FIELD_PAIR(tags),
        FIELD_PAIR(metadata)
    )
    
    TO_JSON_METHODS(User)
};

// 继承类示例 - 自动处理父类字段（推荐）
class AdminUser : public User {
    FIELD(std::string, role)
    FIELD(std::vector<std::string>, permissions)
    
    // 使用 JSON_SERIALIZE_INHERIT 自动处理父类字段
    JSON_SERIALIZE_INHERIT(User,
        FIELD_PAIR(role),
        FIELD_PAIR(permissions)
    )
    
    TO_JSON_METHODS(AdminUser)
};

// 使用方式：
// User user;
// user.set_id(1);
// user.set_name("John");
// Json::Value json = user.to_json();
// std::string json_str = user.to_json_string();
*/
