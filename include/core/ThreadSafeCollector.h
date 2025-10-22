#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

/**
 * @brief Thread-safe data collector (lock-free design)
 *
 * Each thread maintains its own buffer to avoid lock contention
 * Suitable for multi-threaded data collection scenarios, such as intersection detection result collection
 */
template<typename T>
class ThreadSafeCollector {
public:
    /**
     * @brief Constructor
     * @param numThreads Number of threads (0 means auto-detect)
     */
    ThreadSafeCollector(size_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4; // fallback
        }
        m_buffers.resize(numThreads);
    }

    /**
     * @brief Add element to thread-local buffer
     * @param value Value to add
     * @param threadId Thread ID (must be < numThreads)
     */
    void add(const T& value, size_t threadId) {
        if (threadId < m_buffers.size()) {
            m_buffers[threadId].push_back(value);
        }
    }

    /**
     * @brief Add element (auto-detect thread ID)
     * @param value Value to add
     */
    void add(const T& value) {
        size_t threadId = getThreadIndex();
        add(value, threadId);
    }

    /**
     * @brief Collect results from all threads
     * @return Merged result vector
     */
    std::vector<T> collect() const {
        std::vector<T> result;

        // Pre-allocate space
        size_t totalSize = 0;
        for (const auto& buffer : m_buffers) {
            totalSize += buffer.size();
        }
        result.reserve(totalSize);

        // Merge all buffers
        for (const auto& buffer : m_buffers) {
            result.insert(result.end(), buffer.begin(), buffer.end());
        }

        return result;
    }

    /**
     * @brief Clear all buffers
     */
    void clear() {
        for (auto& buffer : m_buffers) {
            buffer.clear();
        }
    }

    /**
     * @brief Get total number of elements
     */
    size_t size() const {
        size_t total = 0;
        for (const auto& buffer : m_buffers) {
            total += buffer.size();
        }
        return total;
    }

    /**
     * @brief Get buffer count
     */
    size_t bufferCount() const {
        return m_buffers.size();
    }

    /**
     * @brief Get size statistics for each buffer
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

    // Thread ID to buffer index mapping
    size_t getThreadIndex() const {
        static thread_local size_t index = SIZE_MAX;

        if (index == SIZE_MAX) {
            // First call: allocate a unique index
            static std::atomic<size_t> counter{0};
            index = counter.fetch_add(1) % m_buffers.size();
        }

        return index;
    }
};

/**
 * @brief Specialized thread-safe collector for geometric data
 * Provides additional geometry-specific functionality
 */
template<typename T>
class GeometryThreadSafeCollector : public ThreadSafeCollector<T> {
public:
    GeometryThreadSafeCollector(size_t numThreads = 0)
        : ThreadSafeCollector<T>(numThreads) {}

    /**
     * @brief Collect and deduplicate (for point data)
     * @param tolerance Deduplication tolerance
     * @return Deduplicated results
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
     * @brief Specialized deduplication collection for geometric points
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
 * @brief Usage example
 */
inline void exampleUsage() {
    // Basic usage
    ThreadSafeCollector<gp_Pnt> collector(4); // 4 threads

    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 1000; ++i) {
        gp_Pnt point(i, i*2, i*3);
        collector.add(point); // Auto-detect thread ID
    }

    auto allPoints = collector.collect();
    // allPoints.size() == 1000

    // Geometry-specific version (with deduplication)
    GeometryThreadSafeCollector<gp_Pnt> geomCollector(4);

    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 1000; ++i) {
        gp_Pnt point(i % 10, (i % 10)*2, (i % 10)*3); // Duplicate points
        geomCollector.add(point);
    }

    auto uniquePoints = geomCollector.collectUniquePoints(1.0);
    // uniquePoints.size() == 10 (after deduplication)
}


