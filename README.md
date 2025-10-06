# Common - C++ 通用工具库

一个功能丰富的C++通用工具库，提供JSON序列化、日志记录、线程池等常用功能模块。

## 📋 目录

- [功能特性](#功能特性)
- [模块介绍](#模块介绍)
- [快速开始](#快速开始)
- [详细文档](#详细文档)
- [依赖项](#依赖项)
- [编译说明](#编译说明)
- [使用示例](#使用示例)
- [贡献指南](#贡献指南)
- [许可证](#许可证)

## 🚀 功能特性

- **JSON序列化/反序列化**: 支持对象与JSON之间的双向转换，支持复杂数据类型
- **日志系统**: 多级别日志记录，支持同步/异步输出，线程安全
- **对象池**: 高效的对象池实现，支持资源复用
- **异步池**: 基于协程的异步对象池
- **类型安全**: 使用现代C++特性，提供类型安全的API
- **跨平台**: 支持Windows、Linux、macOS等主流平台

## 📦 模块介绍

### 1. JsonSerializable 模块
提供完整的JSON序列化和反序列化功能，支持：
- 基础数据类型序列化
- 容器类型（vector、map、set等）序列化
- 嵌套对象序列化
- 继承关系处理
- 可选字段支持

### 2. Log 模块
功能强大的日志系统，特性包括：
- 多级别日志（INFO、WARN、ERROR、DEBUG）
- 自动时间戳和位置信息
- 支持自定义日志输出函数
- 可选的异步日志线程
- 容器类型友好输出

### 3. Pool 模块
提供多种对象池实现：
- **ObjectPool**: 传统对象池，支持对象生命周期管理
- **AsyncPool**: 基于C++20协程的异步对象池

## 🏃‍♂️ 快速开始

### 基本使用

```cpp
#include "JsonSerializable/JsonSerializable.hpp"
#include "Log/Log.hpp"
#include "Pool/ObjectPool.hpp"  // 对象池

// 定义可序列化的类
class User : public JsonSerializable {
    FIELD(int, id)
    FIELD(std::string, name)
    FIELD(std::string, email)
    
    JSON_SERIALIZE_FULL(JsonSerializable, 
        FIELD_PAIR(id),
        FIELD_PAIR(name),
        FIELD_PAIR(email)
    )
    
    JSON_SERIALIZE_COMPLETE(User)
};

int main() {
    // 使用日志
    LOGI() << "应用程序启动";
    
    // 创建用户对象
    User user;
    user.set_id(1);
    user.set_name("张三");
    user.set_email("zhangsan@example.com");
    
    // 序列化为JSON
    std::string json_str = user.to_json_string();
    LOGI() << "用户JSON:" << json_str;
    
    return 0;
}
```

## 📚 详细文档

### [JSON序列化模块文档](docs/JsonSerializable.md)
- 基础用法和高级特性
- 继承关系处理
- 自定义类型支持
- 性能优化建议

### [日志模块文档](docs/Log.md)
- 日志级别和配置
- 异步日志使用
- 自定义输出格式
- 性能考虑

### [对象池模块文档](docs/Pool.md)
- 对象池配置和使用
- 异步池和协程
- 对象生命周期管理
- 最佳实践

## 🔧 依赖项

### 必需依赖
- **C++17** 或更高版本
- **jsoncpp** - JSON处理库

### 可选依赖
- **C++20** - 用于协程支持（AsyncPool模块）

### 安装依赖

#### Ubuntu/Debian
```bash
sudo apt-get install libjsoncpp-dev
```

#### CentOS/RHEL
```bash
sudo yum install jsoncpp-devel
```

#### macOS
```bash
brew install jsoncpp
```

#### Windows (vcpkg)
```bash
vcpkg install jsoncpp
```

## 🛠️ 编译说明

### CMake 配置

```cmake
cmake_minimum_required(VERSION 3.16)
project(Common)

set(CMAKE_CXX_STANDARD 17)

# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 包含头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})

# 添加编译选项
add_compile_options(${JSONCPP_CFLAGS_OTHER})

# 定义宏（可选）
add_definitions(-DJSON_CPP)  # 启用JSON支持
add_definitions(-DLOG_THREAD)  # 启用异步日志
```

### 编译命令

```bash
mkdir build
cd build
cmake ..
make
```

## 💡 使用示例

### JSON序列化示例

```cpp
#include "JsonSerializable/JsonSerializable.hpp"

// 基础类
class Person : public JsonSerializable {
    FIELD(std::string, name)
    FIELD(int, age)
    FIELD(std::vector<std::string>, hobbies)
    
    JSON_SERIALIZE_FULL(JsonSerializable,
        FIELD_PAIR(name),
        FIELD_PAIR(age),
        FIELD_PAIR(hobbies)
    )
    
    JSON_SERIALIZE_COMPLETE(Person)
};

// 继承类
class Employee : public Person {
    FIELD(std::string, department)
    FIELD(double, salary)
    
    JSON_SERIALIZE_FULL_INHERIT(Person,
        FIELD_PAIR(department),
        FIELD_PAIR(salary)
    )
    
    JSON_SERIALIZE_COMPLETE(Employee)
};

// 使用示例
int main() {
    Employee emp;
    emp.set_name("李四");
    emp.set_age(30);
    emp.set_hobbies({"编程", "阅读", "运动"});
    emp.set_department("技术部");
    emp.set_salary(15000.0);
    
    // 序列化
    std::string json = emp.to_json_string();
    std::cout << json << std::endl;
    
    // 反序列化
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(json, root)) {
        auto emp2 = Employee::from_json(root);
        if (emp2) {
            std::cout << "姓名: " << emp2->get_name() << std::endl;
        }
    }
    
    return 0;
}
```

### 日志使用示例

```cpp
#include "Log/Log.hpp"

int main() {
    // 设置自定义日志输出函数
    Log::SetLogWriterFunc([](const std::string& msg) {
        // 写入文件或发送到远程服务器
        std::ofstream file("app.log", std::ios::app);
        file << msg << std::endl;
    });
    
    // 启用异步日志
    Log::StartLogThread();
    
    // 使用不同级别的日志
    LOGI() << "应用程序启动";
    LOGW() << "这是一个警告消息";
    LOGE() << "发生错误:" << "连接超时";
    LOGD() << "调试信息:" << "变量值 = " << 42;
    
    // 记录容器数据
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    LOGI() << "数字列表:" << numbers;
    
    // 停止异步日志
    Log::StopLogThread();
    
    return 0;
}
```

### 对象池使用示例

```cpp
#include "Pool/ObjectPool.hpp"
#include <iostream>

// 自定义连接类
class DatabaseConnection {
public:
    DatabaseConnection() {
        std::cout << "创建数据库连接" << std::endl;
    }
    
    ~DatabaseConnection() {
        std::cout << "销毁数据库连接" << std::endl;
    }
    
    void execute(const std::string& query) {
        std::cout << "执行查询: " << query << std::endl;
    }
};

// 连接池实现
class ConnectionPool : public ObjectPool<DatabaseConnection*> {
public:
    ConnectionPool() : ObjectPool<DatabaseConnection*>(10, 3) {}
    
    DatabaseConnection* Create() noexcept override {
        return new DatabaseConnection();
    }
    
    bool Effective(DatabaseConnection* conn) noexcept override {
        return conn != nullptr;
    }
    
    void Destroy(DatabaseConnection* conn) noexcept override {
        delete conn;
    }
};

int main() {
    ConnectionPool pool;
    
    // 获取连接
    auto conn = pool.Get();
    conn->execute("SELECT * FROM users");
    
    // 归还连接
    pool.Release(conn);
    
    return 0;
}
```

## 🤝 贡献指南

我们欢迎社区贡献！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 代码规范
- 使用4个空格缩进
- 遵循现有的命名约定
- 添加适当的注释（中文）
- 确保代码通过所有测试

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- 提交 [Issue](https://github.com/your-username/Common/issues)
- 发送邮件至: your-email@example.com

---

**注意**: 本项目正在积极开发中，API可能会发生变化。建议在生产环境使用前进行充分测试。
