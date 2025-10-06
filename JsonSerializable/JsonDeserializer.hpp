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
#include "TypeTraits.hpp"
#include "FieldMacros.hpp"

/**
 * JSON 反序列化功能
 * 提供从 JSON 数据初始化对象的能力
 */

// ------------------ 基础类 ------------------
class JsonDeserializer {
public:
    virtual ~JsonDeserializer() = default;
    virtual void from_json(const Json::Value&) {}

    template<typename T, typename... Args>
    void from_json(const Json::Value& j, std::string name, std::optional<T>* value, Args... args) {
        from_json(j, name, value);
        from_json(j, args...);
    }

    template<typename T>
    void from_json(const Json::Value& j, std::string name, std::optional<T>* value) {
        if (value == nullptr || !j.isMember(name)) return;
        
        if constexpr (std::is_base_of_v<JsonDeserializer, T>) {
            T obj; obj.from_json(j[name]); *value = obj;
        } else if constexpr (is_sequence_container<T>::value) {
            T container;
            for (const auto& item : j[name]) {
                typename T::value_type elem;
                if constexpr (std::is_base_of_v<JsonToObject, typename T::value_type>) {
                    elem.from_json(item);
                } else {
                    elem = item.as<typename T::value_type>();
                }
                container.push_back(elem);
            }
            *value = container;
        } else if constexpr (is_set_container<T>::value) {
            T container;
            for (const auto& item : j[name]) {
                typename T::value_type elem;
                if constexpr (std::is_base_of_v<JsonToObject, typename T::value_type>) {
                    elem.from_json(item);
                } else {
                    elem = item.as<typename T::value_type>();
                }
                container.insert(elem);
            }
            *value = container;
        } else if constexpr (is_associative_container<T>::value) {
            T container;
            for (const auto& key : j[name].getMemberNames()) {
                typename T::key_type k = from_string<typename T::key_type>(key);
                typename T::mapped_type v;
                if constexpr (std::is_base_of_v<JsonToObject, typename T::mapped_type>) {
                    v.from_json(j[name][key]);
                } else {
                    v = j[name][key].as<typename T::mapped_type>();
                }
                container[k] = v;
            }
            *value = container;
        } else {
            *value = j[name].as<T>();
        }
    }

    // 辅助方法：从字符串转换键类型
    template<typename K>
    K from_string(const std::string& str) const {
        if constexpr (std::is_same_v<K, std::string>) {
            return str;
        } else if constexpr (std::is_same_v<K, int>) {
            return std::stoi(str);
        } else if constexpr (std::is_same_v<K, long>) {
            return std::stol(str);
        } else if constexpr (std::is_same_v<K, long long>) {
            return std::stoll(str);
        } else if constexpr (std::is_same_v<K, unsigned int>) {
            return std::stoul(str);
        } else if constexpr (std::is_same_v<K, unsigned long>) {
            return std::stoul(str);
        } else if constexpr (std::is_same_v<K, unsigned long long>) {
            return std::stoull(str);
        } else if constexpr (std::is_same_v<K, float>) {
            return std::stof(str);
        } else if constexpr (std::is_same_v<K, double>) {
            return std::stod(str);
        } else {
            return K{};
        }
    }
};

// ------------------ 宏定义 ------------------

// JSON 反序列化宏
#ifndef JSON_DESERIALIZE
#define JSON_DESERIALIZE(BASE, ...) \
public: \
    virtual void from_json(const Json::Value& j) override { \
        BASE::from_json(j, __VA_ARGS__); \
    }
#endif

// JSON 反序列化宏（包含父类字段）
#ifndef JSON_DESERIALIZE_WITH_PARENT
#define JSON_DESERIALIZE_WITH_PARENT(BASE, ...) \
public: \
    virtual void from_json(const Json::Value& j) override { \
        BASE::from_json(j, __VA_ARGS__); \
    }
#endif

// JSON 反序列化宏（自动处理父类字段）
#ifndef JSON_DESERIALIZE_INHERIT
#define JSON_DESERIALIZE_INHERIT(PARENT_CLASS, ...) \
public: \
    virtual void from_json(const Json::Value& j) override { \
        /* 首先调用父类的 from_json 处理父类字段 */ \
        PARENT_CLASS::from_json(j); \
        /* 然后处理子类字段 */ \
        JsonDeserializer::from_json(j, __VA_ARGS__); \
    }
#endif

// 便捷的从 JSON 创建对象的静态方法宏
#ifndef CREATE_FROM_JSON
#define CREATE_FROM_JSON(CLASS_NAME) \
public: \
    static std::optional<CLASS_NAME> from_json(const Json::Value& j) { \
        if (j.isNull()) return std::nullopt; \
        CLASS_NAME obj; \
        obj.from_json(j); \
        return obj; \
    } \
    static std::vector<CLASS_NAME> from_json_array(const Json::Value& j) { \
        std::vector<CLASS_NAME> objects; \
        if (!j.isArray()) return objects; \
        \
        for (const auto& item : j) { \
            CLASS_NAME obj; \
            obj.from_json(item); \
            objects.push_back(std::move(obj)); \
        } \
        return objects; \
    }
#endif

// 使用示例：
/*
// 基础类示例
class User : public JsonDeserializer {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)
    FIELD(std::vector<std::string>, tags)
    FIELD(std::map<std::string, std::string>, metadata)
    
    JSON_DESERIALIZE(JsonDeserializer, 
        FIELD_PAIR(id),
        FIELD_PAIR(name),
        FIELD_PAIR(email),
        FIELD_PAIR(tags),
        FIELD_PAIR(metadata)
    )
    
    CREATE_FROM_JSON(User)
};

// 继承类示例 - 自动处理父类字段（推荐）
class AdminUser : public User {
    FIELD(std::string, role)
    FIELD(std::vector<std::string>, permissions)
    
    // 使用 JSON_DESERIALIZE_INHERIT 自动处理父类字段
    JSON_DESERIALIZE_INHERIT(User,
        FIELD_PAIR(role),
        FIELD_PAIR(permissions)
    )
    
    CREATE_FROM_JSON(AdminUser)
};

// 使用方式：
// Json::Value json = ...; // 从文件或字符串解析
// auto user = User::from_json(json);
// auto users = User::from_json_array(json_array);
*/
