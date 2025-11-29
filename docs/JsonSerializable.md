# JsonSerializable 模块

`JsonSerializable` 提供了一组轻量的辅助宏和基类，用于将 C++ 对象与 `jsoncpp` 的 `Json::Value` 相互转换。

目录
- `JsonSerializable/FieldMacros.h`：定义 `FIELD`、`FIELD_PAIR` 等字段辅助宏
- `JsonSerializable/JsonSerializer.hpp`：对象到 `Json::Value` 的序列化
- `JsonSerializable/JsonDeserializer.hpp`：从 `Json::Value` 到对象的反序列化
- `JsonSerializable/JsonSerializable.hpp`：组合基类与便捷宏
- `JsonSerializable/TypeTraits.h`：类型特性辅助

核心类

```cpp
class JsonSerializable : public JsonDeserializer, public JsonSerializer {
public:
    virtual ~JsonSerializable() = default;
    using JsonDeserializer::from_json;
    using JsonSerializer::to_json;
};
```

主要宏说明

- `JSON_SERIALIZE_FULL(BASE, ...)`
  - 为类生成 `to_json()` 和 `from_json()`，调用指定的 `BASE::to_json` / `BASE::from_json`。

- `JSON_SERIALIZE_FULL_INHERIT(PARENT_CLASS, ...)`
  - 在继承场景中使用：先调用父类的 `to_json()`/`from_json()`，再处理子类字段。

- `JSON_SERIALIZE_COMPLETE(CLASS_NAME)`
  - 为类追加序列化/反序列化便捷函数：`to_json_string()`、`from_json()`（静态）、`from_json_array()`、`to_json_string(vector<>)`。

使用示例

```cpp
class User : public JsonSerializable {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)
    FIELD(std::vector<std::string>, tags)
    FIELD(std::map<std::string, std::string>, metadata)

    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(id), FIELD_PAIR(name), FIELD_PAIR(email), FIELD_PAIR(tags), FIELD_PAIR(metadata)
    )

    JSON_SERIALIZE_COMPLETE(User)
};

// 继承示例
class AdminUser : public User {
    FIELD(std::string, role)
    FIELD(std::vector<std::string>, permissions)

    JSON_SERIALIZE_FULL_INHERIT(User, FIELD_PAIR(role), FIELD_PAIR(permissions))
    JSON_SERIALIZE_COMPLETE(AdminUser)
};

// 使用：
// User u; u.set_id(1); std::string s = u.to_json_string();
// Json::Value j = ...; auto u_opt = User::from_json(j);
```

注意事项

- 这些宏依赖于 `FieldMacros.h` 中的 `FIELD` / `FIELD_PAIR` 等定义，请确保按约定为类添加字段与访问器。
- JSON 支持依赖 `jsoncpp`，若未定义 `JSON_CPP`，相关函数可能不可用。

如需我把这些文档生成更详尽（包括每个宏的展开示例、字段宏定义的完整解释或自动提取字段到文档的脚本），我可以继续扩展。
