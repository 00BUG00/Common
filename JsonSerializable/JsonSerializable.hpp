#pragma once

#include "JsonDeserializer.hpp"
#include "JsonSerializer.hpp"

/**
 * 完整的 JSON 序列化功能
 * 组合了 JSON 到对象和对象到 JSON 的转换功能
 */

// ------------------ 基础类 ------------------
class JsonSerializable : public JsonDeserializer, public JsonSerializer {
public:
    virtual ~JsonSerializable() = default;
    
    // 继承两个基类的功能
    using JsonDeserializer::from_json;
    using JsonSerializer::to_json;
};

// ------------------ 宏定义 ------------------

// 完整的 JSON 序列化宏
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

// 完整的 JSON 序列化宏（包含父类字段）
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

// 完整的 JSON 序列化宏（自动处理父类字段）
#ifndef JSON_SERIALIZE_FULL_INHERIT
#define JSON_SERIALIZE_FULL_INHERIT(PARENT_CLASS, ...) \
public: \
    virtual Json::Value to_json() const override { \
        Json::Value j = PARENT_CLASS::to_json(); \
        JsonSerializer::to_json(j, __VA_ARGS__); \
        return j; \
    } \
    virtual void from_json(const Json::Value& j) override { \
        /* 首先调用父类的 from_json 处理父类字段 */ \
        PARENT_CLASS::from_json(j); \
        /* 然后处理子类字段 */ \
        JsonDeserializer::from_json(j, __VA_ARGS__); \
    }
#endif

// 便捷的完整 JSON 序列化方法宏
#ifndef JSON_SERIALIZE_COMPLETE
#define JSON_SERIALIZE_COMPLETE(CLASS_NAME) \
public: \
    std::string to_json_string() const { \
        Json::Value j = to_json(); \
        Json::StreamWriterBuilder builder; \
        return Json::writeString(builder, j); \
    } \
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
class User : public JsonSerializable {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)
    FIELD(std::vector<std::string>, tags)
    FIELD(std::map<std::string, std::string>, metadata)
    
    JSON_SERIALIZE_FULL(JsonSerializable, 
        FIELD_PAIR(id),
        FIELD_PAIR(name),
        FIELD_PAIR(email),
        FIELD_PAIR(tags),
        FIELD_PAIR(metadata)
    )
    
    JSON_SERIALIZE_COMPLETE(User)
};

// 继承类示例 - 自动处理父类字段（推荐）
class AdminUser : public User {
    FIELD(std::string, role)
    FIELD(std::vector<std::string>, permissions)
    
    // 使用 JSON_SERIALIZE_FULL_INHERIT 自动处理父类字段
    JSON_SERIALIZE_FULL_INHERIT(User,
        FIELD_PAIR(role),
        FIELD_PAIR(permissions)
    )
    
    JSON_SERIALIZE_COMPLETE(AdminUser)
};

// 使用方式：
// 序列化
// User user;
// user.set_id(1);
// user.set_name("John");
// Json::Value json = user.to_json();
// std::string json_str = user.to_json_string();

// 反序列化
// Json::Value json = ...; // 从文件或字符串解析
// auto user = User::from_json(json);
// auto users = User::from_json_array(json_array);
*/
