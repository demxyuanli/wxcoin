#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <functional>
#include <OpenCASCADE/gp_Pnt.hxx>

/**
 * @brief Cache for edge geometry to avoid recomputation
 * 
 * Caches extracted edge points to significantly speed up edge display toggling.
 * Thread-safe singleton implementation.
 */
class EdgeGeometryCache {
public:
    struct CacheEntry {
        std::vector<gp_Pnt> points;
        size_t shapeHash;
        std::chrono::steady_clock::time_point lastAccess;
        size_t memoryUsage;

        CacheEntry() : shapeHash(0), memoryUsage(0) {}
    };

    static EdgeGeometryCache& getInstance() {
        static EdgeGeometryCache instance;
        return instance;
    }

    /**
     * @brief Get cached result or compute if not cached
     * @param key Unique cache key
     * @param computeFunc Function to compute points if cache miss
     * @return Cached or computed edge points
     */
    std::vector<gp_Pnt> getOrCompute(
        const std::string& key,
        std::function<std::vector<gp_Pnt>()> computeFunc);

    /**
     * @brief Invalidate specific cache entry
     * @param key Cache key to invalidate
     */
    void invalidate(const std::string& key);

    /**
     * @brief Clear all cache entries
     */
    void clear();

    /**
     * @brief Remove entries older than specified age
     * @param maxAge Maximum age for cache entries (default: 5 minutes)
     */
    void evictOldEntries(std::chrono::seconds maxAge = std::chrono::seconds(300));

    // Statistics
    size_t getHitCount() const { return m_hitCount; }
    size_t getMissCount() const { return m_missCount; }
    double getHitRate() const;
    size_t getCacheSize() const;
    size_t getTotalMemoryUsage() const;

    /**
     * @brief Estimate memory usage for a vector of points
     */
    size_t estimateMemoryUsage(const std::vector<gp_Pnt>& points);

    /**
     * @brief Check if we should evict entries for a new entry
     */
    bool shouldEvictForNewEntry(size_t newEntrySize);

    /**
     * @brief Evict least recently used entry
     */
    void evictLRU();

private:
    EdgeGeometryCache() : m_hitCount(0), m_missCount(0), m_totalMemoryUsage(0) {}
    EdgeGeometryCache(const EdgeGeometryCache&) = delete;
    EdgeGeometryCache& operator=(const EdgeGeometryCache&) = delete;

    std::unordered_map<std::string, CacheEntry> m_cache;
    mutable std::mutex m_mutex;
    size_t m_hitCount;
    size_t m_missCount;
    size_t m_totalMemoryUsage;
};



