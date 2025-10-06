# 日志模块文档

## 概述

Log模块提供了一个功能强大、易于使用的日志系统，支持多级别日志记录、自定义输出格式、异步日志处理等功能。该模块设计为线程安全，适合多线程环境使用。

## 核心特性

- ✅ 多级别日志（INFO、WARN、ERROR、DEBUG）
- ✅ 自动时间戳和位置信息
- ✅ 线程安全设计
- ✅ 支持自定义日志输出函数
- ✅ 可选的异步日志线程
- ✅ 容器类型友好输出
- ✅ JSON对象支持
- ✅ 跨平台兼容

## 快速开始

### 1. 基础用法

```cpp
#include "Log/Log.hpp"

int main() {
    // 使用不同级别的日志
    LOGI() << "应用程序启动";
    LOGW() << "这是一个警告消息";
    LOGE() << "发生错误:" << "连接超时";
    LOGD() << "调试信息:" << "变量值 = " << 42;
    
    return 0;
}
```

输出示例：
```
2024-01-15 14:30:25 INFO main.cpp[15][main] 应用程序启动
2024-01-15 14:30:25 WARN main.cpp[16][main] 这是一个警告消息
2024-01-15 14:30:25 ERROR main.cpp[17][main] 发生错误: 连接超时
2024-01-15 14:30:25 DEBUG main.cpp[18][main] 调试信息: 变量值 = 42
```

### 2. 容器类型输出

```cpp
#include "Log/Log.hpp"
#include <vector>
#include <map>

int main() {
    // 输出vector
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    LOGI() << "数字列表:" << numbers;
    
    // 输出map
    std::map<std::string, int> scores = {{"张三", 95}, {"李四", 87}};
    LOGI() << "成绩单:" << scores;
    
    return 0;
}
```

输出示例：
```
2024-01-15 14:30:25 INFO main.cpp[8][main] 数字列表: 1,2,3,4,5
2024-01-15 14:30:25 INFO main.cpp[12][main] 数字列表: MAP:{[张三,95], [李四,87]}
```

## 详细API说明

### 日志级别

```cpp
enum class LOG_TYPE {
    INFO,   // 信息日志
    ERROR,  // 错误日志
    WARN,   // 警告日志
    DEBUG,  // 调试日志
};
```

### 日志宏

| 宏 | 级别 | 说明 |
|---|---|---|
| `LOGI()` | INFO | 信息日志，用于记录一般信息 |
| `LOGE()` | ERROR | 错误日志，用于记录错误信息 |
| `LOGW()` | WARN | 警告日志，用于记录警告信息 |
| `LOGD()` | DEBUG | 调试日志，用于开发调试 |

### 日志格式

默认日志格式：
```
YYYY-MM-DD HH:MM:SS LEVEL filename[line][function] message
```

示例：
```
2024-01-15 14:30:25 INFO main.cpp[15][main] 用户登录成功
```

## 高级功能

### 1. 自定义日志输出函数

```cpp
#include "Log/Log.hpp"
#include <fstream>

int main() {
    // 设置自定义日志输出函数
    Log::SetLogWriterFunc([](const std::string& msg) {
        // 写入文件
        std::ofstream file("app.log", std::ios::app);
        file << msg << std::endl;
        
        // 同时输出到控制台
        std::cout << msg << std::endl;
        
        // 可以发送到远程服务器
        // sendToRemoteServer(msg);
    });
    
    LOGI() << "这条日志会同时写入文件和控制台";
    
    return 0;
}
```

### 2. 异步日志（需要定义LOG_THREAD宏）

```cpp
#include "Log/Log.hpp"

// 编译时定义宏启用异步日志
// g++ -DLOG_THREAD your_file.cpp

int main() {
    // 启动异步日志线程
    Log::StartLogThread();
    
    // 大量日志输出不会阻塞主线程
    for (int i = 0; i < 10000; ++i) {
        LOGI() << "日志消息 " << i;
    }
    
    // 停止异步日志线程
    Log::StopLogThread();
    
    return 0;
}
```

### 3. JSON对象支持（需要定义JSON_CPP宏）

```cpp
#include "Log/Log.hpp"
#include <jsoncpp/json/json.h>

// 编译时定义宏启用JSON支持
// g++ -DJSON_CPP your_file.cpp

int main() {
    Json::Value user;
    user["name"] = "张三";
    user["age"] = 30;
    user["city"] = "北京";
    
    // 直接输出JSON对象
    LOGI() << "用户信息:" << user;
    
    return 0;
}
```

输出示例：
```
2024-01-15 14:30:25 INFO main.cpp[12][main] 用户信息:{"age":30,"city":"北京","name":"张三"}
```

## 配置选项

### 编译时配置

| 宏定义 | 说明 | 默认值 |
|---|---|---|
| `LOG_THREAD` | 启用异步日志线程 | 未定义 |
| `JSON_CPP` | 启用JSON对象支持 | 未定义 |

### 编译示例

```bash
# 启用异步日志
g++ -DLOG_THREAD -std=c++17 your_file.cpp

# 启用JSON支持
g++ -DJSON_CPP -std=c++17 your_file.cpp

# 同时启用两个功能
g++ -DLOG_THREAD -DJSON_CPP -std=c++17 your_file.cpp
```

## 性能考虑

### 1. 同步vs异步日志

**同步日志**：
- 优点：简单，无额外线程开销
- 缺点：可能阻塞主线程
- 适用：日志量少，对性能要求不高的场景

**异步日志**：
- 优点：不阻塞主线程，性能更好
- 缺点：需要额外线程，内存使用略高
- 适用：高并发，大量日志输出的场景

### 2. 性能优化建议

```cpp
// 好的做法：避免在循环中频繁创建日志对象
for (int i = 0; i < 1000; ++i) {
    LOGI() << "处理项目 " << i;  // 每次循环都创建Log对象
}

// 更好的做法：批量处理或使用异步日志
Log::StartLogThread();  // 启用异步日志
for (int i = 0; i < 1000; ++i) {
    LOGI() << "处理项目 " << i;  // 异步输出，不阻塞
}
Log::StopLogThread();
```

### 3. 内存使用

- 同步模式：最小内存使用
- 异步模式：需要额外的队列缓冲区
- 建议：根据实际需求选择合适的模式

## 最佳实践

### 1. 日志级别使用

```cpp
// 正确使用不同级别
LOGI() << "用户登录成功";           // 一般信息
LOGW() << "磁盘空间不足";           // 警告信息
LOGE() << "数据库连接失败";         // 错误信息
LOGD() << "变量值: " << debug_var;  // 调试信息
```

### 2. 结构化日志

```cpp
// 使用结构化信息
LOGI() << "用户操作" 
       << "用户ID:" << user_id 
       << "操作:" << operation 
       << "结果:" << result;
```

### 3. 错误处理

```cpp
try {
    // 可能出错的操作
    risky_operation();
} catch (const std::exception& e) {
    LOGE() << "操作失败:" << e.what();
    // 错误处理逻辑
}
```

### 4. 条件日志

```cpp
// 只在调试模式下输出详细日志
#ifdef DEBUG
    LOGD() << "详细调试信息:" << detailed_info;
#endif
```

## 常见问题

### Q: 如何控制日志输出格式？
A: 当前版本使用固定格式。如需自定义格式，可以：
1. 使用自定义日志输出函数
2. 修改Log类的实现
3. 在输出函数中进行格式转换

### Q: 异步日志线程何时启动和停止？
A: 建议：
- 在程序初始化时启动：`Log::StartLogThread()`
- 在程序退出前停止：`Log::StopLogThread()`
- 确保所有日志都已输出后再停止

### Q: 如何处理日志文件轮转？
A: 可以通过自定义日志输出函数实现：
```cpp
Log::SetLogWriterFunc([](const std::string& msg) {
    // 检查文件大小，实现轮转逻辑
    static std::ofstream file("app.log", std::ios::app);
    if (file.tellp() > MAX_FILE_SIZE) {
        file.close();
        // 重命名或删除旧文件
        file.open("app.log", std::ios::app);
    }
    file << msg << std::endl;
});
```

### Q: 多线程环境下是否安全？
A: 是的，Log类设计为线程安全：
- 使用互斥锁保护共享资源
- 异步模式下使用条件变量进行线程同步
- 可以安全地在多个线程中使用

## 示例项目

完整的使用示例请参考项目根目录下的`examples/`文件夹。

## 更新日志

- **v1.0.0**: 初始版本，支持基础日志功能
- **v1.1.0**: 添加异步日志支持
- **v1.2.0**: 添加JSON对象支持
- **v1.3.0**: 性能优化，改进线程安全
