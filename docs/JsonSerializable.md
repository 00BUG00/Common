# JSON序列化模块文档

## 概述

JsonSerializable模块提供了完整的JSON序列化和反序列化功能，支持C++对象与JSON数据之间的双向转换。该模块基于jsoncpp库，提供了类型安全、易于使用的API。

## 核心特性

- ✅ 支持基础数据类型（int、string、double等）
- ✅ 支持STL容器（vector、map、set、list等）
- ✅ 支持嵌套对象序列化
- ✅ 支持继承关系处理
- ✅ 支持可选字段（std::optional）
- ✅ 提供便捷的宏定义
- ✅ 类型安全的API设计

## 模块结构

```
JsonSerializable/
├── JsonSerializable.hpp    # 主头文件，组合序列化和反序列化功能
├── JsonSerializer.hpp      # JSON序列化功能
├── JsonDeserializer.hpp    # JSON反序列化功能
├── TypeTraits.hpp          # 类型特征检测
└── FieldMacros.hpp         # 字段宏定义
```

## 快速开始

### 1. 基础用法

```cpp
#include "JsonSerializable/JsonSerializable.hpp"

class User : public JsonSerializable {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)
    FIELD(std::vector<std::string>, tags)
    
    JSON_SERIALIZE_FULL(JsonSerializable, 
        FIELD_PAIR(id),
        FIELD_PAIR(name),
        FIELD_PAIR(email),
        FIELD_PAIR(tags)
    )
    
    JSON_SERIALIZE_COMPLETE(User)
};

// 使用示例
int main() {
    User user;
    user.set_id(1);
    user.set_name("张三");
    user.set_email("zhangsan@example.com");
    user.set_tags({"开发者", "C++"});
    
    // 序列化为JSON字符串
    std::string json_str = user.to_json_string();
    std::cout << json_str << std::endl;
    
    // 从JSON反序列化
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(json_str, root)) {
        auto user2 = User::from_json(root);
        if (user2) {
            std::cout << "用户ID: " << user2->get_id() << std::endl;
        }
    }
    
    return 0;
}
```

### 2. 继承关系处理

```cpp
// 基类
class Person : public JsonSerializable {
    FIELD(std::string, name)
    FIELD(int, age)
    
    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(name),
        FIELD_PAIR(age)
    )
    
    JSON_SERIALIZE_COMPLETE(Person)
};

// 派生类 - 自动处理父类字段
class Employee : public Person {
    FIELD(std::string, department)
    FIELD(double, salary)
    FIELD(std::vector<std::string>, skills)
    
    // 使用 JSON_SERIALIZE_FULL_INHERIT 自动处理父类字段
    JSON_SERIALIZE_FULL_INHERIT(Person,
        FIELD_PAIR(department),
        FIELD_PAIR(salary),
        FIELD_PAIR(skills)
    )
    
    JSON_SERIALIZE_COMPLETE(Employee)
};
```

## 详细API说明

### 宏定义

#### 1. FIELD 宏
用于定义类的成员变量，自动生成getter和setter方法。

```cpp
FIELD(type, name)  // 定义类型为type，名称为name的字段
```

#### 2. FIELD_PAIR 宏
用于在序列化宏中指定字段对。

```cpp
FIELD_PAIR(name)  // 在序列化宏中使用，指定要序列化的字段
```

#### 3. JSON_SERIALIZE_FULL 宏
提供完整的序列化和反序列化功能。

```cpp
JSON_SERIALIZE_FULL(BASE, ...)  // BASE为基类，...为字段列表
```

#### 4. JSON_SERIALIZE_FULL_INHERIT 宏
自动处理继承关系的序列化。

```cpp
JSON_SERIALIZE_FULL_INHERIT(PARENT_CLASS, ...)  // 自动处理父类字段
```

#### 5. JSON_SERIALIZE_COMPLETE 宏
提供便捷的静态方法。

```cpp
JSON_SERIALIZE_COMPLETE(CLASS_NAME)  // 添加便捷的静态方法
```

### 支持的数据类型

#### 基础类型
- `int`, `long`, `long long`
- `unsigned int`, `unsigned long`, `unsigned long long`
- `float`, `double`, `long double`
- `bool`
- `std::string`
- `std::optional<T>`

#### 容器类型
- `std::vector<T>`
- `std::list<T>`
- `std::deque<T>`
- `std::set<T>`
- `std::unordered_set<T>`
- `std::map<K, V>`
- `std::unordered_map<K, V>`

#### 自定义类型
- 继承自`JsonSerializable`的类
- 继承自`JsonSerializer`的类

## 高级用法

### 1. 可选字段处理

```cpp
class Product : public JsonSerializable {
    FIELD(std::string, name)
    FIELD(double, price)
    FIELD(std::optional<std::string>, description)  // 可选字段
    FIELD(std::optional<int>, stock_count)          // 可选字段
    
    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(name),
        FIELD_PAIR(price),
        FIELD_PAIR(description),
        FIELD_PAIR(stock_count)
    )
    
    JSON_SERIALIZE_COMPLETE(Product)
};
```

### 2. 嵌套对象序列化

```cpp
class Address : public JsonSerializable {
    FIELD(std::string, street)
    FIELD(std::string, city)
    FIELD(std::string, country)
    
    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(street),
        FIELD_PAIR(city),
        FIELD_PAIR(country)
    )
    
    JSON_SERIALIZE_COMPLETE(Address)
};

class Company : public JsonSerializable {
    FIELD(std::string, name)
    FIELD(Address, address)  // 嵌套对象
    FIELD(std::vector<Employee>, employees)  // 对象数组
    
    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(name),
        FIELD_PAIR(address),
        FIELD_PAIR(employees)
    )
    
    JSON_SERIALIZE_COMPLETE(Company)
};
```

### 3. 数组处理

```cpp
// 序列化对象数组
std::vector<User> users = {user1, user2, user3};
std::string json_array = User::to_json_string(users);

// 从JSON数组反序列化
Json::Value root;
Json::Reader reader;
if (reader.parse(json_array, root)) {
    auto users = User::from_json_array(root);
    for (const auto& user : users) {
        std::cout << "用户: " << user.get_name() << std::endl;
    }
}
```

### 4. 自定义序列化逻辑

```cpp
class CustomClass : public JsonSerializable {
    FIELD(std::string, data)
    FIELD(int, secret_value)
    
    // 自定义序列化逻辑
    virtual Json::Value to_json() const override {
        Json::Value j;
        j["data"] = get_data();
        // 不序列化secret_value，或者进行特殊处理
        j["has_secret"] = get_secret_value() > 0;
        return j;
    }
    
    virtual void from_json(const Json::Value& j) override {
        if (j.isMember("data")) {
            set_data(j["data"].asString());
        }
        // 根据has_secret字段设置secret_value
        if (j.isMember("has_secret") && j["has_secret"].asBool()) {
            set_secret_value(1);
        }
    }
};
```

## 性能优化建议

### 1. 避免频繁的JSON字符串转换
```cpp
// 好的做法：直接使用Json::Value
Json::Value json = user.to_json();
// 进行其他操作...

// 避免：频繁的字符串转换
std::string json_str = user.to_json_string();
Json::Value json;
Json::Reader reader;
reader.parse(json_str, json);  // 不必要的解析
```

### 2. 使用引用避免拷贝
```cpp
// 好的做法：使用const引用
void processUser(const User& user) {
    Json::Value json = user.to_json();
    // 处理...
}

// 避免：不必要的拷贝
void processUser(User user) {  // 拷贝构造
    // 处理...
}
```

### 3. 预分配容器大小
```cpp
// 如果知道大概大小，预分配可以提高性能
std::vector<User> users;
users.reserve(1000);  // 预分配空间
```

## 常见问题

### Q: 如何处理循环引用？
A: 当前版本不支持循环引用。如果遇到循环引用，建议：
1. 使用ID引用而不是直接对象引用
2. 在序列化时忽略循环引用的字段
3. 使用自定义序列化逻辑处理

### Q: 如何处理版本兼容性？
A: 建议：
1. 使用可选字段处理新增字段
2. 在反序列化时检查字段存在性
3. 提供默认值处理缺失字段

### Q: 性能如何？
A: 性能特点：
- 序列化性能：与jsoncpp相当
- 内存使用：最小化临时对象创建
- 编译时间：使用模板，编译时间略长

## 示例项目

完整的使用示例请参考项目根目录下的`examples/`文件夹。

## 更新日志

- **v1.0.0**: 初始版本，支持基础序列化功能
- **v1.1.0**: 添加继承关系支持
- **v1.2.0**: 添加可选字段支持
- **v1.3.0**: 性能优化，添加便捷方法
