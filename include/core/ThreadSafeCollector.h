#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

/**
 * @brief 线程安全的数据收集器（无锁设计）
 *
 * 每个线程维护独立的缓冲区，避免锁竞争
 * 适合多线程数据收集场景，如交点检测结果收集
 */
template<typename T>
class ThreadSafeCollector {
public:
    /**
     * @brief 构造函数
     * @param numThreads 线程数量（0表示自动检测）
     */
    ThreadSafeCollector(size_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4; // fallback
        }
        m_buffers.resize(numThreads);
    }

    /**
     * @brief 添加元素到线程本地缓冲区
     * @param value 要添加的值
     * @param threadId 线程ID（必须<numThreads）
     */
    void add(const T& value, size_t threadId) {
        if (threadId < m_buffers.size()) {
            m_buffers[threadId].push_back(value);
        }
    }

    /**
     * @brief 添加元素（自动检测线程ID）
     * @param value 要添加的值
     */
    void add(const T& value) {
        size_t threadId = getThreadIndex();
        add(value, threadId);
    }

    /**
     * @brief 收集所有线程的结果
     * @return 合并后的结果向量
     */
    std::vector<T> collect() const {
        std::vector<T> result;

        // 预分配空间
        size_t totalSize = 0;
        for (const auto& buffer : m_buffers) {
            totalSize += buffer.size();
        }
        result.reserve(totalSize);

        // 合并所有缓冲区
        for (const auto& buffer : m_buffers) {
            result.insert(result.end(), buffer.begin(), buffer.end());
        }

        return result;
    }

    /**
     * @brief 清空所有缓冲区
     */
    void clear() {
        for (auto& buffer : m_buffers) {
            buffer.clear();
        }
    }

    /**
     * @brief 获取总元素数
     */
    size_t size() const {
        size_t total = 0;
        for (const auto& buffer : m_buffers) {
            total += buffer.size();
        }
        return total;
    }

    /**
     * @brief 获取缓冲区数量
     */
    size_t bufferCount() const {
        return m_buffers.size();
    }

    /**
     * @brief 获取每个缓冲区的大小统计
     */
    std::vector<size_t> getBufferSizes() const {
        std::vector<size_t> sizes;
        sizes.reserve(m_buffers.size());
        for (const auto& buffer : m_buffers) {
            sizes.push_back(buffer.size());
        }
        return sizes;
    }

private:
    std::vector<std::vector<T>> m_buffers;

    // 线程ID到缓冲区索引的映射
    size_t getThreadIndex() const {
        static thread_local size_t index = SIZE_MAX;

        if (index == SIZE_MAX) {
            // 首次调用：分配一个唯一索引
            static std::atomic<size_t> counter{0};
            index = counter.fetch_add(1) % m_buffers.size();
        }

        return index;
    }
};

/**
 * @brief 专门用于几何数据的线程安全收集器
 * 提供额外的几何特定功能
 */
template<typename T>
class GeometryThreadSafeCollector : public ThreadSafeCollector<T> {
public:
    GeometryThreadSafeCollector(size_t numThreads = 0)
        : ThreadSafeCollector<T>(numThreads) {}

    /**
     * @brief 收集并去重（适用于点数据）
     * @param tolerance 去重容差
     * @return 去重后的结果
     */
    template<typename DistanceFunc>
    std::vector<T> collectUnique(DistanceFunc distanceFunc, double tolerance) const {
        auto allResults = this->collect();
        std::vector<T> uniqueResults;

        for (const auto& item : allResults) {
            bool isUnique = true;
            for (const auto& existing : uniqueResults) {
                if (distanceFunc(item, existing) < tolerance) {
                    isUnique = false;
                    break;
                }
            }
            if (isUnique) {
                uniqueResults.push_back(item);
            }
        }

        return uniqueResults;
    }

    /**
     * @brief 几何点专用去重收集
     */
    template<typename PointType = T>
    std::enable_if_t<std::is_same_v<PointType, gp_Pnt>, std::vector<gp_Pnt>>
    collectUniquePoints(double tolerance) const {
        return collectUnique([](const gp_Pnt& a, const gp_Pnt& b) {
            return a.Distance(b);
        }, tolerance);
    }
};

/**
 * @brief 使用示例
 */
inline void exampleUsage() {
    // 基本用法
    ThreadSafeCollector<gp_Pnt> collector(4); // 4个线程

    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 1000; ++i) {
        gp_Pnt point(i, i*2, i*3);
        collector.add(point); // 自动检测线程ID
    }

    auto allPoints = collector.collect();
    // allPoints.size() == 1000

    // 几何专用版本（带去重）
    GeometryThreadSafeCollector<gp_Pnt> geomCollector(4);

    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 1000; ++i) {
        gp_Pnt point(i % 10, (i % 10)*2, (i % 10)*3); // 重复点
        geomCollector.add(point);
    }

    auto uniquePoints = geomCollector.collectUniquePoints(1.0);
    // uniquePoints.size() == 10 (去重后)
}


