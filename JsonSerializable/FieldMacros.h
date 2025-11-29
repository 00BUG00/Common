#pragma once

#include <optional>
#include <map>
#include <unordered_map>

/**
 * 字段定义宏
 * 用于简化类的字段定义和访问器生成
 */

// 基础字段定义宏
#ifndef FIELD
#define FIELD(TYPE, NAME) \
private: std::optional<TYPE> _##NAME; \
public: \
    const std::optional<TYPE>& get_##NAME() const { return _##NAME; } \
    void set_##NAME(const TYPE& value) { _##NAME = value; } \
    void reset_##NAME() { _##NAME.reset(); }
#endif

// 有序映射字段定义宏
#ifndef FIELD_MAP
#define FIELD_MAP(KEY_TYPE, VALUE_TYPE, NAME) \
private: std::optional<std::map<KEY_TYPE, VALUE_TYPE>> _##NAME; \
public: \
    const std::optional<std::map<KEY_TYPE, VALUE_TYPE>>& get_##NAME() const { return _##NAME; } \
    void set_##NAME(const std::map<KEY_TYPE, VALUE_TYPE>& value) { _##NAME = value; } \
    void reset_##NAME() { _##NAME.reset(); }
#endif

// 无序映射字段定义宏
#ifndef FIELD_UNORDERED_MAP
#define FIELD_UNORDERED_MAP(KEY_TYPE, VALUE_TYPE, NAME) \
private: std::optional<std::unordered_map<KEY_TYPE, VALUE_TYPE>> _##NAME; \
public: \
    const std::optional<std::unordered_map<KEY_TYPE, VALUE_TYPE>>& get_##NAME() const { return _##NAME; } \
    void set_##NAME(const std::unordered_map<KEY_TYPE, VALUE_TYPE>& value) { _##NAME = value; } \
    void reset_##NAME() { _##NAME.reset(); }
#endif

// 字段对宏（用于序列化/反序列化参数传递）
#ifndef FIELD_PAIR
#define FIELD_PAIR(NAME) #NAME, &_##NAME
#endif