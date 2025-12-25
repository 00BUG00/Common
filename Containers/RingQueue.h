#include <atomic>
#include <vector>
#include <cstddef>
#include <cstdint>

enum class RingQueueResult {
    Ok = 0,
    Full,
    Empty,
    Busy,
};

/**
 * @class RingQueue
 * @brief 一个协程和线程都安全的环形队列
 * @details 该类提供了一个线程安全和协程安全的环形队列实现，适用于多线程或多协程环境下的数据交换。
 * @author BUG
 * @date 2025-12-22
 */

template<typename T>
class RingQueue {
public:
    /**
     * @brief 构造函数
     * @details 初始话化一个指定容量的环形队列。
     * @param capacity 队列中可容纳的最大元素数量。
     * @return 无
     * @author BUG
     * @date 2025-12-22
     */
    explicit RingQueue(size_t capacity)
        : nodes_(capacity)
    {
        for (size_t i = 0; i < capacity; ++i) {
            nodes_[i].sequence.store(i, std::memory_order_relaxed);
        }
        _head.store(0, std::memory_order_relaxed);
        _tail.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief 析构函数
     * @details 释放环形队列所占用的资源。析构前必须保证没有并发访问
     * @return 无
     * @author BUG
     * @date 2025-12-22
     */
    ~RingQueue() = default;

    /**
     * @brief 尝试将元素推入队列（非阻塞）
     *
     * @details
     * - 该函数不会阻塞线程或协程
     * - 若当前发生并发竞争或 slot 尚未就绪，会立即返回 Busy
     * - 调用者可选择重试、让出执行权或结合外部同步机制
     *
     * @param item 要推入的元素
     *
     * @return
     * - RingQueueResult::Ok    推入成功
     * - RingQueueResult::Full  队列已满
     * - RingQueueResult::Busy  并发竞争失败，可重试
     *
     * @thread_safety 线程安全 / 协程安全（非阻塞）
     * @author BUG
     * @date 2025-12-22
     */
    inline RingQueueResult TryPush(const T& item) {
        size_t tail = _tail.load(std::memory_order_relaxed);
        size_t head = _head.load(std::memory_order_acquire);

        // 快速满判断（hint）
        if (tail - head >= nodes_.size()) {
            return RingQueueResult::Full;
        }

        size_t index = tail % nodes_.size();
        Node& node = nodes_[index];

        size_t seq = node.sequence.load(std::memory_order_acquire);
        intptr_t diff =
            static_cast<intptr_t>(seq) - static_cast<intptr_t>(tail);

        if (diff != 0) {
            if (diff < 0) return RingQueueResult::Full;
            return RingQueueResult::Busy;
        }

        if (!_tail.compare_exchange_strong(
                tail, tail + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return RingQueueResult::Busy;
        }

        node.data = item;
        node.sequence.store(tail + 1, std::memory_order_release);
        return RingQueueResult::Ok;
    }
    
    /**
     * @brief 尝试从队列弹出一个元素（非阻塞）
     *
     * @details
     * - 该函数不会阻塞线程或协程
     * - 若当前发生并发竞争或 slot 尚未准备好，会立即返回 Busy
     * - 若队列为空，返回 Empty
     *
     * @param item 用于接收弹出元素的引用
     *
     * @return
     * - RingQueueResult::Ok     弹出成功
     * - RingQueueResult::Empty  队列为空
     * - RingQueueResult::Busy   并发竞争失败，可重试
     *
     * @author BUG
     * @date 2025-12-22
     */
    inline RingQueueResult TryPop(T& item) {
        size_t head = _head.load(std::memory_order_relaxed);
        size_t tail = _tail.load(std::memory_order_acquire);

        if (head == tail) {
            return RingQueueResult::Empty;
        }

        size_t index = head % nodes_.size();
        Node& node = nodes_[index];

        size_t seq = node.sequence.load(std::memory_order_acquire);
        if (seq != head + 1) {
            if (_head.load(std::memory_order_relaxed)
                == _tail.load(std::memory_order_relaxed)) {
                return RingQueueResult::Empty;
            }
            return RingQueueResult::Busy;
        }

        if (!_head.compare_exchange_strong(
                head, head + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return RingQueueResult::Busy;
        }

        item = node.data;
        node.sequence.store(
            head + nodes_.size(),
            std::memory_order_release);

        return RingQueueResult::Ok;
    }

    /**
     * @brief 获取内部队列的可用空间（近似值）
     * @details
     * - 返回值为近似值（Approximate）
     * - 仅用于监控、统计或调试
     * - 不应作为业务逻辑判断依据
     *
     * @return 返回可用空间大小 类型为 size_t
     * @author BUG
     * @date 2025-12-22
     */
    inline size_t SizeApprox() const {
        size_t head = _head.load(std::memory_order_acquire);
        size_t tail = _tail.load(std::memory_order_acquire);
        return tail - head;
    }

    /**
     * @brief 获取内部队列的容量
     * @details 返回环形队列的总容量，即它可以容纳的最大元素数量。
     * @return 返回容量大小 类型为size_t
     * @author BUG
     * @date 2025-12-22
     */
    inline size_t Capacity() const {
        return nodes_.size();
    }

    /**
     * @brief 获取内部队列的可用空间
     * @details 返回环形队列当前可用的空间大小。
     * @return 返回可用空间大小 类型为size_t
     * @author BUG
     * @date 2025-12-22
     */
    inline size_t AvailableApprox() const {
        return nodes_.size() - SizeApprox();
    }

    /**
     * @brief 判断队列是否为空（近似判断）
     *
     * @details
     * 该函数用于判断队列当前是否为空的【近似状态】。
     * 在并发（多线程 / 多协程）环境下：
     *
     * - 返回值不是严格一致的
     * - 在函数返回后，队列状态可能已发生变化
     * - 该函数不与 TryPush / TryPop 构成原子语义
     *
     * 适用场景：
     * - 快速状态探测
     * - 调试 / 监控
     * - 作为调度或统计的参考信息
     *
     * 不适用场景：
     * - 作为业务逻辑的唯一判断条件
     * - 替代 TryPop 的返回值判断
     *
     * 并发语义：
     * - 使用 acquire 语义读取 head / tail
     * - 仅保证读取到的值在内存可见性上是自洽的
     *
     * @return
     * - true  表示队列“可能为空”
     * - false 表示队列“可能非空”
     *
     * @note
     * 若需要严格语义，请使用 TryPop 并检查其返回值
     *
     * @author BUG
     * @date 2025-12-22
     */
    inline bool IsEmptyApprox() const {
        size_t head = _head.load(std::memory_order_acquire);
        size_t tail = _tail.load(std::memory_order_acquire);
        return head == tail;
    }

    /**
     * @brief 判断队列是否已满（近似判断）
     *
     * @details
     * 该函数用于判断队列当前是否已满的【近似状态】。
     * 在并发（多线程 / 多协程）环境下：
     *
     * - 返回值不是严格一致的
     * - 在函数返回后，队列状态可能已发生变化
     * - 该函数不与 TryPush / TryPop 构成原子语义
     *
     * 适用场景：
     * - 快速状态探测
     * - 调试 / 监控
     * - 限流或背压的参考信号
     *
     * 不适用场景：
     * - 作为业务逻辑的唯一判断条件
     * - 替代 TryPush 的返回值判断
     *
     * 并发语义：
     * - 使用 acquire 语义读取 head / tail
     * - 仅保证读取到的值在内存可见性上是自洽的
     *
     * @return
     * - true  表示队列“可能已满”
     * - false 表示队列“可能未满”
     *
     * @note
     * 若需要严格语义，请使用 TryPush 并检查其返回值
     *
     * @author BUG
     * @date 2025-12-22
     */
    inline bool IsFullApprox() const {
        size_t head = _head.load(std::memory_order_acquire);
        size_t tail = _tail.load(std::memory_order_acquire);
        return (tail - head) >= nodes_.size();
    }

private:
    struct Node {
        T data;
        std::atomic<size_t> sequence;
    };

    std::atomic<size_t> _head;
    std::atomic<size_t> _tail;
    std::vector<Node> nodes_;
};
