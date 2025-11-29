# Common

根说明已更新：所有模块的详细说明请参阅各模块目录下的 `README.md`，仓内源码为唯一权威。

模块索引（代码为准）
- `Log/README.md` — 日志模块（基于 `Log/Log.hpp`）。
- `Pool/README.md` — 消费者/派发模块（基于 `Pool/ThreadConsumer.hpp`、`Pool/CoroutineConsumer.hpp`）。
- `JsonSerializable/README.md` — JSON 序列化辅助（基于 `JsonSerializable/`）。

如果你在 README 中仍看到 `LoggerManager` 相关示例或说明，请确保你打开的是最新文件（运行 `git status` / `git log -n 5 --oneline`），或告诉我我将把所有 `LoggerManager` 相关行彻底移除并提交一次替换。


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
