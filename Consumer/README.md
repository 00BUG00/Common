# Pool 模块

位置: `Pool/ThreadConsumer.hpp`, `Pool/CoroutineConsumer.hpp`

本文件夹的 `README.md` 是该模块的权威说明，内容严格基于目录下的实现。

核心说明
- `ThreadConsumer<T>`：基于线程的通用消费者模板。
  - 构造：`ThreadConsumer(Callback func, int threadCount = 1)`，`Callback = std::function<void(const T&)>`。
  - 方法：`Start()`, `AddTask(const T&)`, `Stop()`。
  - 行为：`AddTask` 在 `_running == false` 时会忽略任务；`Stop()` 会清空队列并等待线程退出。

- `CoroutineConsumer<T>`：基于 C++20 协程的消费者，实现了自定义 awaiter 与事件循环（需要编译器支持协程）。

示例：ThreadConsumer

```cpp
#include "Pool/ThreadConsumer.hpp"
#include <iostream>

struct Job { int id; std::string msg; };

int main(){
    ThreadConsumer<Job> c([](const Job& j){ std::cout << j.id << ": " << j.msg << std::endl; }, 2);
    c.Start();
    c.AddTask({1, "hello"});
    c.AddTask({2, "world"});
    c.Stop();
    return 0;
}
```

示例：CoroutineConsumer（需 C++20 协程支持）

```cpp
#include "Pool/CoroutineConsumer.hpp"
#include <iostream>

int main(){
    CoroutineConsumer<int> c([](const int& v){ std::cout << "recv: " << v << std::endl; }, 2);
    c.Start();
    c.AddTask(10);
    c.AddTask(20);
    c.Stop();
    return 0;
}
```

注意
- `CoroutineConsumer` 依赖编译器对协程的支持。事件循环通过条件变量唤醒等待协程。
