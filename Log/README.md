# Log 模块

位置: `Log/Log.hpp`

本文件夹的 `README.md` 是该模块的权威说明，内容严格基于 `Log/Log.hpp` 的实现。

核心说明
- 日志宏：`LOGI()`、`LOGW()`、`LOGE()`、`LOGD()`。使用宏会临时创建 `Log` 对象，允许使用 `operator<<` 进行流式拼接。
- 日志数据：`LogData`（包含 `LOG_TYPE`、文件、行号、函数、本地时间、内容）。
- 自定义写回调：`Log::SetLogWriterFunc(std::function<void(const LogData)>)`。若未设置回调，`Log` 的析构会将日志字符串打印到 `std::cout`。
- 类型友好输出：对可迭代容器、KV 容器与 `Json::Value`（若启用 `JSON_CPP`）有专门的 `operator<<` 重载。

示例（完整、可编译）

```cpp
#include "Log/Log.hpp"
#include <iostream>

int main() {
    Log::SetLogWriterFunc([](const LogData& d){
        std::cout << Log::ToString(d) << std::endl;
    });

    LOGI() << "Started" << 123;
    LOGW() << "Warning: " << std::vector<int>{1,2,3};
    return 0;
}
```

注意
- 回调实现必须保证线程安全（回调可能在任意线程/上下文被触发）。
- 若你需要一个全局管理器（例如 `LoggerManager`），请明确需求，我可以实现并把接口与文档提交为单独 commit；目前仓内并无该实现。
