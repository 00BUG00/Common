# JsonSerializable 模块

位置: `JsonSerializable/`（`FieldMacros.h`, `JsonSerializer.hpp`, `JsonDeserializer.hpp`, `JsonSerializable.hpp`, `TypeTraits.h`）

说明
- `JsonSerializable` 组合了 `JsonSerializer` 与 `JsonDeserializer`，提供 `to_json()` 与 `from_json()`。具体实现和宏位于本目录下。
- 主要宏：`JSON_SERIALIZE_FULL`、`JSON_SERIALIZE_FULL_INHERIT`、`JSON_SERIALIZE_COMPLETE`。
- 使用 `FIELD` / `FIELD_PAIR` 等宏声明字段、生成访问器并在序列化宏中引用它们。

示例

```cpp
#include "JsonSerializable/JsonSerializable.hpp"

class User : public JsonSerializable {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)

    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(id), FIELD_PAIR(name), FIELD_PAIR(email)
    )

    JSON_SERIALIZE_COMPLETE(User)
};
```

注意
- 这些宏依赖 `FieldMacros.h` 的字段与访问器约定，请在使用前阅读该头文件。
- JSON 功能依赖 `jsoncpp`（可选），在使用相关功能时请确保链接该库并定义 `JSON_CPP`。
