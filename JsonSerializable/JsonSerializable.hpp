#pragma once

#include "JsonDeserializer.hpp"
#include "JsonSerializer.hpp"

/**
 * @brief 完整的 JSON 序列化基类
 * @details 组合了 JSON -> 对象（反序列化）和 对象 -> JSON（序列化）的功能。
 * 适用于需要同时支持序列化和反序列化的对象。
 */
class JsonSerializable : public JsonDeserializer, public JsonSerializer {
public:
    virtual ~JsonSerializable() = default;

    // 显式继承两个基类的接口
    using JsonDeserializer::from_json;
    using JsonSerializer::to_json;
};

// ========================= 宏定义 =========================

/**
 * @brief 完整 JSON 序列化宏
 * @param BASE 父类名
 * @param ... 要序列化/反序列化的字段名称
 * @details 自动生成对象的 to_json 和 from_json 方法
 */
#ifndef JSON_SERIALIZE_FULL
#define JSON_SERIALIZE_FULL(BASE, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j; BASE::to_json(j, __VA_ARGS__); return j; \
    } \
    virtual void from_json(const Json::Value& j) override { \
        BASE::from_json(j, __VA_ARGS__); \
    }
#endif

/**
 * @brief 完整 JSON 序列化宏（包含父类字段）
 * @param BASE 父类名
 * @param ... 要序列化/反序列化的字段名称
 * @details 与 JSON_SERIALIZE_FULL 功能类似，可显式指定父类字段
 */
#ifndef JSON_SERIALIZE_FULL_WITH_PARENT
#define JSON_SERIALIZE_FULL_WITH_PARENT(BASE, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j; BASE::to_json(j, __VA_ARGS__); return j; \
    } \
    virtual void from_json(const Json::Value& j) override { \
        BASE::from_json(j, __VA_ARGS__); \
    }
#endif

/**
 * @brief 完整 JSON 序列化宏（自动处理父类字段）
 * @param PARENT_CLASS 父类名
 * @param ... 要序列化/反序列化的字段名称
 * @details
 *  1. 调用父类的 to_json/from_json 自动处理父类字段
 *  2. 再处理子类字段
 *  适合继承结构的类使用
 */
#ifndef JSON_SERIALIZE_FULL_INHERIT
#define JSON_SERIALIZE_FULL_INHERIT(PARENT_CLASS, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j = PARENT_CLASS::to_json(); \
        JsonSerializer::to_json(j, __VA_ARGS__); \
        return j; \
    } \
    virtual void from_json(const Json::Value& j) override { \
        /* 先调用父类的 from_json 处理父类字段 */ \
        PARENT_CLASS::from_json(j); \
        /* 再处理子类字段 */ \
        JsonDeserializer::from_json(j, __VA_ARGS__); \
    }
#endif

/**
 * @brief 便捷完整 JSON 序列化方法宏
 * @param CLASS_NAME 类名
 * @details
 *  提供：
 *   - 对象 to_json_string()
 *   - 从 JSON 创建对象 from_json()
 *   - 从 JSON 数组创建对象数组 from_json_array()
 *   - 将对象数组序列化为 JSON 字符串 to_json_string(vector)
 */
#ifndef JSON_SERIALIZE_COMPLETE
#define JSON_SERIALIZE_COMPLETE(CLASS_NAME) \
public: \
    /** 序列化当前对象为 JSON 字符串 */ \
    std::string to_json_string() const { \
        Json::Value j = to_json(); \
        Json::StreamWriterBuilder builder; \
        return Json::writeString(builder, j); \
    } \
    /** 从 JSON 创建单个对象 */ \
    static std::optional<CLASS_NAME> from_json(const Json::Value& j) { \
        if (j.isNull()) return std::nullopt; \
        CLASS_NAME obj; \
        obj.from_json(j); \
        return obj; \
    } \
    /** 从 JSON 数组创建对象数组 */ \
    static std::vector<CLASS_NAME> from_json_array(const Json::Value& j) { \
        std::vector<CLASS_NAME> objects; \
        if (!j.isArray()) return objects; \
        for (const auto& item : j) { \
            CLASS_NAME obj; \
            obj.from_json(item); \
            objects.push_back(std::move(obj)); \
        } \
        return objects; \
    } \
    /** 将对象数组序列化为 JSON 字符串 */ \
    static std::string to_json_string(const std::vector<CLASS_NAME>& objects) { \
        Json::Value arr(Json::arrayValue); \
        for (const auto& obj : objects) { \
            arr.append(obj.to_json()); \
        } \
        Json::StreamWriterBuilder builder; \
        return Json::writeString(builder, arr); \
    }
#endif
