# Common

公共组件集合（C++）：包含日志、JSON 序列化助手与生产者-消费者工具。

特点概览
- 轻量日志系统（宏、LogData、回调机制）——实现位于 `Log/Log.hpp`。
- JSON 序列化/反序列化宏与基类——实现位于 `JsonSerializable/`（依赖 `jsoncpp` 可选）。
- 消费者实现：线程版与协程版——位于 `Pool/ThreadConsumer.hpp` 与 `Pool/CoroutineConsumer.hpp`。

目录结构
```
Common/
    Log/
        Log.hpp              # 日志实现（宏、LogData、回调）
        test_log.cpp         # 示例/测试
    JsonSerializable/
        FieldMacros.h
        JsonDeserializer.hpp
        JsonSerializer.hpp
        JsonSerializable.hpp
        TypeTraits.h
    Pool/
        ThreadConsumer.hpp   # 基于线程的消费者模板
        CoroutineConsumer.hpp# 基于 C++20 协程的消费者模板
    docs/
        Log.md
        Pool.md
        JsonSerializable.md
    README.md
```

兼容性与依赖
- 建议使用支持 C++17 的编译器；若要使用 `CoroutineConsumer`，需支持 C++20 协程。
- JSON 支持为可选：若定义 `JSON_CPP` 并链接 `jsoncpp`，`Log` 与 `JsonSerializable` 中会启用 JSON 支持。

快速示例

日志（基于 `Log/Log.hpp`）
```cpp
#include "Log/Log.hpp"

int main() {
    LOGI() << "应用程序启动";
    Log::SetLogWriterFunc([](const LogData& d){
        // 默认 ToString + 控制台
        std::cout << Log::ToString(d) << std::endl;
    });
    LOGW() << "警告示例";
}
```

将日志交由线程消费者处理（示例）
```cpp
#include "Pool/ThreadConsumer.hpp"
#include "Log/Log.hpp"

int main() {
    ThreadConsumer<LogData> c([](const LogData& d){
        std::cout << Log::ToString(d) << std::endl;
    }, 1);
    c.Start();
    Log::SetLogWriterFunc([&c](const LogData& d){ c.AddTask(d); });
    LOGI() << "异步日志示例";
    c.Stop();
}
```

JSON 可序列化对象（`JsonSerializable`）
```cpp
#include "JsonSerializable/JsonSerializable.hpp"

// 在类中使用 FIELD 宏和 JSON_SERIALIZE_* 宏以启用自动序列化
```

文档
- `docs/Log.md`：基于 `Log/Log.hpp` 的使用说明与注意事项。
- `docs/Pool.md`：基于 `Pool` 下两个消费者实现的说明与示例。
- `docs/JsonSerializable.md`：描述 `JsonSerializable` 基类与宏的使用。

下一步
- 如果你希望项目包含一个全局 `LoggerManager`（单例）以统一管理 Logger，我可以根据现有文档草案实现一个头文件/实现并把示例整合回来；或者我们保持当前简洁的基于 `Log::SetLogWriterFunc` 的集成方式。


# Common - C++ 通用工具库

一个功能丰富的C++通用工具库，提供 JSON 序列化、日志记录、线程/协程派发等常用功能模块。该库采用现代 C++ 设计，强调线程安全、易用性与高效性。

## 📋 目录

- [功能特性](#功能特性)
- [模块介绍](#模块介绍)
- [快速开始](#快速开始)
- [核心架构](#核心架构)
- [详细文档](#详细文档)
- [依赖项](#依赖项)
- [编译说明](#编译说明)
- [使用示例](#使用示例)

## 🚀 功能特性

- **JSON 序列化/反序列化**: 支持对象与 JSON 之间的双向转换，支持复杂数据类型和继承关系
- **日志系统**: 多级别日志（INFO、WARN、ERROR、DEBUG）、自动时间戳与位置信息、容器友好输出
- **灵活的日志派发**: 
  - **全局线程消费** (`GLOBAL_THREAD`): 单一线程消费所有日志
  - **本地线程消费** (`LOCAL_THREAD`): 每个 Logger 独立维护消费线程
  - **全局协程调度** (`GLOBAL_CORO`, C++20+): 协程驱动的日志派发
  - **本地协程派发** (`LOCAL_CORO`, C++20+): 每个 Logger 独立协程派发
- **线程池 / 消费者**: 支持多线程任务队列、线程安全的数据派发
- **类型安全**: 使用现代 C++ 特性（模板、SFINAE、C++20 协程）
- **跨平台**: 支持 Windows、Linux、macOS 等主流平台

## 📦 模块介绍

### 1. JsonSerializable 模块
提供完整的 JSON 序列化和反序列化功能，支持：
- 基础数据类型序列化（int、string、double 等）
- STL 容器类型（vector、map、set、list 等）
- 嵌套对象和继承关系处理
- std::optional 可选字段支持
- 便捷的宏定义（`FIELD`、`JSON_SERIALIZE_FULL`、`JSON_SERIALIZE_COMPLETE` 等）

### 2. Log 模块
功能强大的日志系统，特性包括：
- 多级别日志（INFO、WARN、ERROR、DEBUG）与对应宏（`LOGI()`、`LOGW()`、`LOGE()`、`LOGD()`）
- 自动时间戳和文件/行号/函数名信息
- 支持自定义日志输出回调函数
- 容器类型友好输出（自动格式化 vector、map 等）
- **全局 LoggerManager** 单例：
  - 统一管理多个模块的 Logger 实例
  - 按模块名称创建/获取 Logger
  - 统一设置派发模式和回调
  - 支持 GLOBAL_THREAD / LOCAL_THREAD / 协程派发等模式

### 3. Pool 模块
提供多种对象管理和数据派发实现：
- **ThreadConsumer**: 通用的多线程消费者模板，支持任意数据类型
- **CoroutineConsumer**: 基于 C++20 协程的消费者（待实现）
- **ObjectPool**: 传统对象池（可选）
- **AsyncPool**: 基于协程的异步池（可选）

## 🏃‍♂️ 快速开始

### 1. 基本日志使用

```cpp
#include "Log/LoggerManager.h"

int main() {
    // 获取全局 LoggerManager
    auto& mgr = LoggerManager::GetLoggerManager();
    
    // 获取或创建默认 Logger
    auto* logger = mgr.GetLogger();
    
    // 使用日志宏
    LOGI() << "应用程序启动";
    LOGW() << "这是一个警告";
    LOGE() << "发生错误: " << "某种错误";
    LOGD() << "调试信息: 变量值 = " << 42;
    
    return 0;
}
```

### 2. JSON 序列化

```cpp
#include "JsonSerializable/JsonSerializable.hpp"
#include "Log/LoggerManager.h"

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
    auto& mgr = LoggerManager::GetLoggerManager();
    auto* logger = mgr.GetLogger("User");
    
    // 创建用户对象
    User user;
    user.set_id(1);
    user.set_name("张三");
    user.set_email("zhangsan@example.com");
    
    // 序列化为JSON
    std::string json_str = user.to_json_string();
    LOGI() << "用户JSON: " << json_str;
    
    return 0;
}
```

### 3. 线程派发与日志消费

```cpp
#include "Log/LoggerManager.h"

int main() {
    auto& mgr = LoggerManager::GetLoggerManager();
    
    // 创建自定义日志回调
    auto log_callback = [](const LogData& log) {
        std::cout << Log::ToString(log) << std::endl;
        // 或写入文件、发送到远程等
    };
    
    // 为所有 Logger 设置回调
    mgr.SetWriteCallback(log_callback);
    
    // 启动日志派发器（全局线程模式）
    mgr.Start(LOG_DISPATCH_MODE::GLOBAL_THREAD);
    
    // ... 你的应用代码 ...
    LOGI() << "执行中的日志";
    
    // 程序退出前停止派发器
    mgr.Stop();
    
    return 0;
}
```

## 🏗️ 核心架构

### LoggerManager - 全局单例管理器

**LoggerManager** 是日志系统的中枢，负责：
- 创建和维护多个模块的 `Logger` 实例
- 统一管理日志派发模式（全局线程 / 本地线程 / 协程等）
- 为所有 Logger 设置统一的写入回调
- 生命周期管理：析构时停止并释放所有 Logger

**关键方法**：
- `GetLoggerManager()`: 获取全局单例
- `GetLogger(name)`: 获取或创建指定模块的 Logger（懒创建）
- `InsertLogger(name)`: 主动创建新的 Logger
- `SetWriteCallback(callback, name)`: 设置日志写入回调
- `Start(mode)`: 启动日志派发（支持 GLOBAL_THREAD 等模式）
- `Stop()`: 停止派发并清理资源

### Logger - 模块级日志实例

每个 Logger 负责一个模块的日志记录，支持：
- 接收 `LogData` 日志数据
- 按 `LOG_DISPATCH_MODE` 选择派发方式
- 调用配置的写入回调

### 日志派发模式 (LOG_DISPATCH_MODE)

| 模式 | 说明 |
|------|------|
| `GLOBAL_THREAD` | 所有日志进入全局队列，由单一线程消费 |
| `LOCAL_THREAD` | 每个 Logger 维护独立队列和消费线程 |
| `GLOBAL_CORO` (C++20+) | 全局队列由协程调度器消费 |
| `LOCAL_CORO` (C++20+) | 每个 Logger 使用独立协程派发 |

## 📚 详细文档

### [JSON 序列化模块文档](docs/JsonSerializable.md)
- 完整 API 与宏定义说明
- 基础用法、继承关系处理
- 嵌套对象、容器类型支持
- 性能优化建议

### [日志系统文档](docs/Log.md)
- LoggerManager 单例设计与用法
- 日志级别和格式配置
- 日志派发方式详解 (GLOBAL_THREAD / LOCAL_THREAD / 协程模式)
- 自定义回调与日志输出
- 线程安全保证和使用建议

### [线程消费与对象池文档](docs/Pool.md)
- ThreadConsumer 多线程派发器用法
- CoroutineConsumer 协程派发（C++20+）
- 对象池与资源管理

## 📋 依赖项

- **C++17** 或更高版本
- **jsoncpp** 库（可选，仅在启用 JSON 支持时）
- **C++20 编译器**（可选，用于协程特性 `GLOBAL_CORO`、`LOCAL_CORO`）

## 🔨 编译说明

### CMake 构建（推荐）

```bash
mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD=17 ..
make
```

### 启用 C++20 协程

```bash
cmake -DCMAKE_CXX_STANDARD=20 ..
make
```

### 编译选项

| 选项 | 说明 | 默认值 |
|------|------|-------|
| `CMAKE_CXX_STANDARD` | C++ 标准版本 (17/20) | 17 |
| `ENABLE_TESTS` | 编译单元测试 | OFF |
| `ENABLE_EXAMPLES` | 编译示例代码 | OFF |

## 📝 使用示例

### 示例 1: 日志系统与 LoggerManager

```cpp
#include "Log/LoggerManager.h"
#include <iostream>
#include <fstream>

int main() {
    auto& mgr = LoggerManager::GetLoggerManager();
    
    // 创建两个模块的 Logger
    mgr.InsertLogger("Network");
    mgr.InsertLogger("Database");
    
    // 设置写入回调（同时输出到控制台和文件）
    mgr.SetWriteCallback([](const LogData& log) {
        std::string msg = Log::ToString(log);
        std::cout << msg << std::endl;
        
        std::ofstream file("app.log", std::ios::app);
        file << msg << std::endl;
    });
    
    // 启动全局线程派发器
    mgr.Start(LOG_DISPATCH_MODE::GLOBAL_THREAD);
    
    // 获取不同模块的 Logger
    auto* net_logger = mgr.GetLogger("Network");
    auto* db_logger = mgr.GetLogger("Database");
    
    // 记录日志（自动派发到消费线程）
    LOGI() << "网络模块启动";
    LOGW() << "数据库连接缓慢";
    LOGE() << "连接失败";
    
    // 停止派发器
    mgr.Stop();
    
    return 0;
}
```

### 示例 2: JSON 序列化与反序列化

```cpp
#include "JsonSerializable/JsonSerializable.hpp"
#include "Log/LoggerManager.h"

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

int main() {
    User user;
    user.set_id(1);
    user.set_name("张三");
    user.set_email("zhangsan@example.com");
    user.set_tags({"C++", "开发"});
    
    // 序列化为 JSON 字符串
    std::string json_str = user.to_json_string();
    LOGI() << "用户 JSON: " << json_str;
    
    // 从 JSON 反序列化
    auto user_opt = User::from_json_string(json_str);
    if (user_opt) {
        LOGI() << "恢复用户: " << user_opt->get_name();
    }
    
    return 0;
}
```

### 示例 3: ThreadConsumer 多线程派发

```cpp
#include "Pool/ThreadConsumer.hpp"
#include <iostream>
#include <chrono>

struct Task {
    int id;
    std::string message;
};

int main() {
    // 创建消费者：处理任务的回调函数，启动 4 个消费线程
    ThreadConsumer<Task> consumer(
        [](const Task& task) {
            std::cout << "处理任务 #" << task.id << ": " << task.message << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        },
        4  // 4 个线程
    );
    
    consumer.Start();
    
    // 添加任务
    for (int i = 0; i < 10; ++i) {
        consumer.AddTask({i, "Task " + std::to_string(i)});
    }
    
    // 停止消费者（等待当前任务完成）
    consumer.Stop();
    
    return 0;
}
```
