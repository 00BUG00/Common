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
 * @brief JSON 反序列化基类
 * @details 提供从 JSON 数据初始化对象的能力，支持基本类型、STL 容器、可选类型以及继承自 JsonDeserializer 的自定义类型。
 * 使用方法：
 * 1. 继承 JsonDeserializer。
 * 2. 重载 from_json() 或使用宏自动生成反序列化函数。
 * 3. 调用 from_json() 或静态方法 from_json() / from_json_array() 获取对象或对象数组。
 */
class JsonDeserializer {
public:
    virtual ~JsonDeserializer() = default;

    /**
     * @brief 从 JSON 对象初始化当前对象
     * @param j JSON 对象
     */
    virtual void from_json(const Json::Value&) {}

    // ========================= 可选类型反序列化 =========================

    /**
     * @brief 将多个可选字段从 JSON 对象中反序列化
     * @tparam T 第一个字段类型
     * @tparam Args 其他字段类型
     * @param j JSON 对象引用
     * @param name 当前字段名称
     * @param value 当前字段的可选值指针
     * @param args 其他字段参数包
     */
    template<typename T, typename... Args>
    void from_json(const Json::Value& j, std::string name, std::optional<T>* value, Args... args) {
        from_json(j, name, value);
        from_json(j, args...);
    }

    /**
     * @brief 将单个可选字段从 JSON 对象中反序列化
     * @tparam T 字段类型
     * @param j JSON 对象引用
     * @param name 字段名称
     * @param value 字段的可选值指针
     */
    template<typename T>
    void from_json(const Json::Value& j, std::string name, std::optional<T>* value) {
        if (value == nullptr || !j.isMember(name)) return;
        
        if constexpr (std::is_base_of_v<JsonDeserializer, T>) {
            T obj; 
            obj.from_json(j[name]);
            *value = obj;
        } else if constexpr (is_sequence_container<T>::value) {
            T container;
            for (const auto& item : j[name]) {
                typename T::value_type elem;
                if constexpr (std::is_base_of_v<JsonDeserializer, typename T::value_type>) {
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
                if constexpr (std::is_base_of_v<JsonDeserializer, typename T::value_type>) {
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
                if constexpr (std::is_base_of_v<JsonDeserializer, typename T::mapped_type>) {
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

    // ========================= 辅助方法 =========================

    /**
     * @brief 从字符串转换键类型
     * @tparam K 键类型
     * @param str 字符串表示的键
     * @return 转换后的键值
     */
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

// ========================= 宏定义 =========================

/**
 * @brief 自动生成 JSON 反序列化函数宏
 * @param BASE 父类名
 * @param ... 要反序列化的字段名称
 */
#ifndef JSON_DESERIALIZE
#define JSON_DESERIALIZE(BASE, ...) \
public: \
    virtual void from_json(const Json::Value& j) override { \
        BASE::from_json(j, __VA_ARGS__); \
    }
#endif

/**
 * @brief 自动生成 JSON 反序列化函数宏（包含父类字段）
 * @param BASE 父类名
 * @param ... 要反序列化的字段名称
 */
#ifndef JSON_DESERIALIZE_WITH_PARENT
#define JSON_DESERIALIZE_WITH_PARENT(BASE, ...) \
public: \
    virtual void from_json(const Json::Value& j) override { \
        BASE::from_json(j, __VA_ARGS__); \
    }
#endif

/**
 * @brief 自动生成 JSON 反序列化函数宏（继承父类字段并反序列化自身字段）
 * @param PARENT_CLASS 父类名
 * @param ... 要反序列化的字段名称
 */
#ifndef JSON_DESERIALIZE_INHERIT
#define JSON_DESERIALIZE_INHERIT(PARENT_CLASS, ...) \
public: \
    virtual void from_json(const Json::Value& j) override { \
        /* 先调用父类的 from_json 处理父类字段 */ \
        PARENT_CLASS::from_json(j); \
        /* 再处理子类字段 */ \
        JsonDeserializer::from_json(j, __VA_ARGS__); \
    }
#endif

/**
 * @brief 为类提供便捷的静态反序列化方法宏
 * @param CLASS_NAME 类名
 * @details 提供：
 *  - 静态方法 from_json() 返回 std::optional<CLASS_NAME>
 *  - 静态方法 from_json_array() 返回 std::vector<CLASS_NAME>
 */
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
        for (const auto& item : j) { \
            CLASS_NAME obj; \
            obj.from_json(item); \
            objects.push_back(std::move(obj)); \
        } \
        return objects; \
    }
#endif
