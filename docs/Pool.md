# 对象池模块文档

## 概述

Pool模块提供了多种对象池实现，包括传统对象池、异步对象池等。这些池类可以帮助管理资源生命周期，提高应用程序性能，减少对象创建和销毁的开销。

## 核心特性

- ✅ 线程安全的对象池管理
- ✅ 自动对象生命周期管理
- ✅ 支持自定义创建、验证、销毁逻辑
- ✅ 基于C++20协程的异步池
- ✅ 可配置的池大小限制
- ✅ 高效的资源复用机制

## 模块结构

```
Pool/
├── ObjectPool.hpp          # 传统对象池实现
└── AsyncPool.hpp           # 基于协程的异步池
```

## 快速开始

### 1. 基础对象池使用

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

## 详细API说明

### ObjectPool 类（传统对象池）

#### 构造函数
```cpp
ObjectPool(int max_size = 100, int size = 10)
```
- `max_size`: 池中最大对象数量
- `size`: 初始创建的对象数量

#### 核心方法

| 方法 | 说明 |
|---|---|
| `T Get()` | 从池中获取一个对象，如果池为空且未达到最大数量则创建新对象 |
| `void Release(T item)` | 将对象归还到池中 |
| `void Clear()` | 清空池，销毁所有对象 |
| `virtual T Create() noexcept = 0` | 纯虚函数，子类必须实现对象创建逻辑 |
| `virtual bool Effective(T item) noexcept = 0` | 纯虚函数，子类必须实现对象有效性检查 |
| `virtual void Destroy(T item) noexcept = 0` | 纯虚函数，子类必须实现对象销毁逻辑 |

### AsyncPool 类（异步对象池，C++20协程）

#### 构造函数
```cpp
AsyncPool(int max_size = 10, int size = 1)
```

#### 协程支持
```cpp
AsyncPoolAwaiter<T> Get()  // 返回协程等待器
```

## 使用示例

### 1. 数据库连接池

```cpp
#include "Pool/ObjectPool.hpp"
#include <mysql/mysql.h>

class MySQLConnection {
private:
    MYSQL* _mysql;
    
public:
    MySQLConnection() {
        _mysql = mysql_init(nullptr);
        if (!mysql_real_connect(_mysql, "localhost", "user", "password", "database", 3306, nullptr, 0)) {
            throw std::runtime_error("数据库连接失败");
        }
    }
    
    ~MySQLConnection() {
        if (_mysql) {
            mysql_close(_mysql);
        }
    }
    
    bool isConnected() const {
        return _mysql != nullptr && mysql_ping(_mysql) == 0;
    }
    
    MYSQL* getHandle() { return _mysql; }
};

class MySQLPool : public ObjectPool<MySQLConnection*> {
public:
    MySQLPool() : ObjectPool<MySQLConnection*>(20, 5) {}
    
    MySQLConnection* Create() noexcept override {
        try {
            return new MySQLConnection();
        } catch (...) {
            return nullptr;
        }
    }
    
    bool Effective(MySQLConnection* conn) noexcept override {
        return conn != nullptr && conn->isConnected();
    }
    
    void Destroy(MySQLConnection* conn) noexcept override {
        delete conn;
    }
};

// 使用示例
int main() {
    MySQLPool pool;
    
    // 获取连接
    auto conn = pool.Get();
    if (conn) {
        // 执行数据库操作
        MYSQL* mysql = conn->getHandle();
        // ... 数据库操作 ...
        
        // 归还连接
        pool.Release(conn);
    }
    
    return 0;
}
```

### 2. HTTP连接池

```cpp
#include "Pool/ObjectPool.hpp"
#include <curl/curl.h>

class HttpClient {
private:
    CURL* _curl;
    
public:
    HttpClient() {
        _curl = curl_easy_init();
        if (!_curl) {
            throw std::runtime_error("CURL初始化失败");
        }
    }
    
    ~HttpClient() {
        if (_curl) {
            curl_easy_cleanup(_curl);
        }
    }
    
    bool isValid() const {
        return _curl != nullptr;
    }
    
    CURL* getHandle() { return _curl; }
};

class HttpPool : public ObjectPool<HttpClient*> {
public:
    HttpPool() : ObjectPool<HttpClient*>(50, 10) {}
    
    HttpClient* Create() noexcept override {
        try {
            return new HttpClient();
        } catch (...) {
            return nullptr;
        }
    }
    
    bool Effective(HttpClient* client) noexcept override {
        return client != nullptr && client->isValid();
    }
    
    void Destroy(HttpClient* client) noexcept override {
        delete client;
    }
};
```

### 3. 异步池使用（C++20）

```cpp
#include "Pool/AsyncPool.hpp"
#include <coroutine>
#include <iostream>

// 自定义资源类
class NetworkResource {
public:
    NetworkResource() {
        std::cout << "创建网络资源" << std::endl;
    }
    
    ~NetworkResource() {
        std::cout << "销毁网络资源" << std::endl;
    }
    
    void use() {
        std::cout << "使用网络资源" << std::endl;
    }
};

// 异步池实现
class NetworkPool : public AsyncPool<NetworkResource*> {
public:
    NetworkPool() : AsyncPool<NetworkResource*>(10, 2) {}
    
    NetworkResource* Create() override {
        return new NetworkResource();
    }
    
    bool Effective(NetworkResource* resource) override {
        return resource != nullptr;
    }
    
    void Destroy(NetworkResource* resource) override {
        delete resource;
    }
};

// 协程函数
std::coroutine_handle<> async_task(NetworkPool& pool) {
    // 异步获取资源
    auto resource = co_await pool.GetAsync();
    resource->use();
    
    // 归还资源
    pool.Put(resource);
}

int main() {
    NetworkPool pool;
    
    // 启动协程
    auto handle = async_task(pool);
    handle.resume();
    
    // 等待完成
    while (!handle.done()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
```

## 高级用法

### 1. 自定义对象验证

```cpp
class CustomPool : public ObjectPool<MyObject*> {
public:
    CustomPool() : ObjectPool<MyObject*>(100, 20) {}
    
    MyObject* Create() noexcept override {
        try {
            auto obj = new MyObject();
            // 初始化对象
            obj->initialize();
            return obj;
        } catch (...) {
            return nullptr;
        }
    }
    
    bool Effective(MyObject* obj) noexcept override {
        if (!obj) return false;
        
        // 检查对象状态
        if (!obj->isValid()) return false;
        
        // 检查对象是否过期
        if (obj->isExpired()) return false;
        
        // 重置对象状态
        obj->reset();
        
        return true;
    }
    
    void Destroy(MyObject* obj) noexcept override {
        if (obj) {
            obj->cleanup();
            delete obj;
        }
    }
};
```

### 2. 池状态监控

```cpp
class MonitoredPool : public ObjectPool<MyObject*> {
private:
    std::atomic<int> _created_count{0};
    std::atomic<int> _destroyed_count{0};
    std::atomic<int> _current_size{0};
    
public:
    MonitoredPool() : ObjectPool<MyObject*>(50, 10) {}
    
    MyObject* Create() noexcept override {
        _created_count++;
        _current_size++;
        return new MyObject();
    }
    
    bool Effective(MyObject* obj) noexcept override {
        return obj != nullptr;
    }
    
    void Destroy(MyObject* obj) noexcept override {
        _destroyed_count++;
        _current_size--;
        delete obj;
    }
    
    // 监控方法
    int getCreatedCount() const { return _created_count.load(); }
    int getDestroyedCount() const { return _destroyed_count.load(); }
    int getCurrentSize() const { return _current_size.load(); }
    
    void printStats() const {
        std::cout << "池统计信息:" << std::endl;
        std::cout << "  已创建: " << getCreatedCount() << std::endl;
        std::cout << "  已销毁: " << getDestroyedCount() << std::endl;
        std::cout << "  当前大小: " << getCurrentSize() << std::endl;
    }
};
```

### 3. 异常安全处理

```cpp
class SafePool : public ObjectPool<MyObject*> {
public:
    SafePool() : ObjectPool<MyObject*>(100, 20) {}
    
    MyObject* Create() noexcept override {
        try {
            return new MyObject();
        } catch (const std::bad_alloc&) {
            // 内存分配失败
            return nullptr;
        } catch (const std::exception&) {
            // 其他异常
            return nullptr;
        } catch (...) {
            // 未知异常
            return nullptr;
        }
    }
    
    bool Effective(MyObject* obj) noexcept override {
        try {
            return obj != nullptr && obj->isValid();
        } catch (...) {
            // 验证过程中发生异常，认为对象无效
            return false;
        }
    }
    
    void Destroy(MyObject* obj) noexcept override {
        try {
            if (obj) {
                obj->cleanup();
                delete obj;
            }
        } catch (...) {
            // 销毁过程中发生异常，忽略
        }
    }
};
```

## 性能优化建议

### 1. 合理设置池大小

```cpp
// 根据实际需求设置池大小
class OptimizedPool : public ObjectPool<MyObject*> {
public:
    OptimizedPool() : ObjectPool<MyObject*>(
        100,  // 最大大小：根据内存限制设置
        20    // 初始大小：根据预期并发量设置
    ) {}
};
```

### 2. 避免频繁的池操作

```cpp
// 好的做法：批量处理
void processBatch(MyPool& pool, const std::vector<Task>& tasks) {
    auto obj = pool.Get();
    for (const auto& task : tasks) {
        obj->process(task);
    }
    pool.Release(obj);
}

// 避免：频繁获取和释放
void processBatch(MyPool& pool, const std::vector<Task>& tasks) {
    for (const auto& task : tasks) {
        auto obj = pool.Get();  // 频繁获取
        obj->process(task);
        pool.Release(obj);      // 频繁释放
    }
}
```

### 3. 使用RAII管理资源

```cpp
template<typename T, typename Pool>
class PooledObject {
private:
    T* _obj;
    Pool* _pool;
    
public:
    PooledObject(Pool& pool) : _pool(&pool) {
        _obj = _pool->Get();
    }
    
    ~PooledObject() {
        if (_obj && _pool) {
            _pool->Release(_obj);
        }
    }
    
    T* get() { return _obj; }
    T* operator->() { return _obj; }
    T& operator*() { return *_obj; }
    
    // 禁止拷贝
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;
    
    // 允许移动
    PooledObject(PooledObject&& other) noexcept 
        : _obj(other._obj), _pool(other._pool) {
        other._obj = nullptr;
        other._pool = nullptr;
    }
};

// 使用示例
void usePooledObject(MyPool& pool) {
    PooledObject<MyObject, MyPool> obj(pool);
    obj->doSomething();
    // 自动归还到池中
}
```

## 常见问题

### Q: 如何选择合适的池大小？
A: 考虑因素：
- 内存限制
- 预期并发量
- 对象创建成本
- 系统负载

建议：从较小的初始值开始，根据实际使用情况调整。

### Q: 池中的对象何时被销毁？
A: 对象在以下情况被销毁：
- 调用`Clear()`方法时
- 对象被验证为无效时
- 池析构时

### Q: 如何处理对象创建失败？
A: `Create()`方法应该返回`nullptr`表示创建失败，池会自动处理这种情况。

### Q: 异步池和传统池的区别？
A: 
- 传统池：同步获取，适合传统多线程应用
- 异步池：基于协程，适合异步编程模式

### Q: 如何实现对象池的监控？
A: 可以：
1. 继承池类并添加统计信息
2. 使用回调函数记录池操作
3. 定期输出池状态信息

## 最佳实践

### 1. 资源管理
- 使用RAII模式管理资源生命周期
- 及时归还不需要的对象
- 避免长时间持有池对象

### 2. 异常处理
- 在`Create()`、`Effective()`、`Destroy()`方法中处理异常
- 使用`noexcept`标记确保异常安全

### 3. 性能考虑
- 合理设置池大小
- 避免频繁的池操作
- 使用对象复用减少创建开销

### 4. 线程安全
- 池类本身是线程安全的
- 确保自定义的对象操作也是线程安全的

## 示例项目

完整的使用示例请参考项目根目录下的`examples/`文件夹。

## 更新日志

- **v1.0.0**: 初始版本，支持基础线程池功能
- **v1.1.0**: 添加异步池支持
- **v1.2.0**: 性能优化，改进资源管理
- **v1.3.0**: 添加监控和统计功能
