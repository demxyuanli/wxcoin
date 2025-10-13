#include "edges/EdgeGeometryCache.h"
#include "logger/Logger.h"
#include <sstream>
#include <algorithm>

std::vector<gp_Pnt> EdgeGeometryCache::getOrCompute(
    const std::string& key,
    std::function<std::vector<gp_Pnt>()> computeFunc)
{
    // First, check if key exists in cache (with lock)
    bool cacheHit = false;
    std::vector<gp_Pnt> cachedPoints;
    size_t pointsSize = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // Cache hit - update access time and get cached data
            it->second.lastAccess = std::chrono::steady_clock::now();
            m_hitCount++;
            cacheHit = true;
            cachedPoints = it->second.points;
            pointsSize = cachedPoints.size();
        } else {
            // Cache miss - increment counter
            m_missCount++;
        }
    }
    // Lock is released here

    // Log after releasing lock to avoid potential deadlock
    if (cacheHit) {
        LOG_DBG_S("EdgeCache HIT: " + key + " (points: " + std::to_string(pointsSize) + ")");
        return cachedPoints;
    }

    LOG_DBG_S("EdgeCache MISS: " + key + " (computing...)");

    // Compute new data WITHOUT holding the lock
    // This prevents recursive locking if computeFunc accesses the cache
    auto points = computeFunc();

    // Re-acquire lock to insert into cache
    size_t cacheSize = 0;
    bool insertedNew = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Double-check: another thread might have computed and cached it while we were computing
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // Another thread already cached it, use that version
            return it->second.points;
        }

        // Check memory usage before inserting
        size_t estimatedMemory = estimateMemoryUsage(points);
        if (shouldEvictForNewEntry(estimatedMemory)) {
            evictLRU();
        }

        CacheEntry entry;
        entry.points = points;
        entry.shapeHash = 0;
        entry.lastAccess = std::chrono::steady_clock::now();
        entry.memoryUsage = estimatedMemory;

        m_cache[key] = std::move(entry);
        m_totalMemoryUsage += estimatedMemory;
        cacheSize = m_cache.size();
        insertedNew = true;
    }

    return points;
}

void EdgeGeometryCache::invalidate(const std::string& key) {
    bool found = false;
    size_t freedMemory = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            freedMemory = it->second.memoryUsage;
            m_cache.erase(it);
            m_totalMemoryUsage -= freedMemory;
            found = true;
        }
    }

    if (found) {
        LOG_DBG_S("EdgeCache invalidated: " + key + " (freed: " + std::to_string(freedMemory) + " bytes)");
    }
}

void EdgeGeometryCache::clear() {
    size_t oldSize = 0;
    size_t freedMemory = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        oldSize = m_cache.size();
        freedMemory = m_totalMemoryUsage;
        m_cache.clear();
        m_totalMemoryUsage = 0;
        m_hitCount = 0;
        m_missCount = 0;
    }

    LOG_DBG_S("EdgeCache cleared: " + std::to_string(oldSize) + " entries (" + std::to_string(freedMemory) + " bytes)");
}

void EdgeGeometryCache::evictOldEntries(std::chrono::seconds maxAge) {
    size_t evicted = 0;
    size_t freedMemory = 0;
    size_t remaining = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto now = std::chrono::steady_clock::now();

        for (auto it = m_cache.begin(); it != m_cache.end();) {
            if (now - it->second.lastAccess > maxAge) {
                freedMemory += it->second.memoryUsage;
                it = m_cache.erase(it);
                evicted++;
            } else {
                ++it;
            }
        }

        m_totalMemoryUsage -= freedMemory;
        remaining = m_cache.size();
    }

    if (evicted > 0) {
        LOG_DBG_S("EdgeCache evicted: " + std::to_string(evicted) + " old entries (" + std::to_string(freedMemory) + " bytes), " + std::to_string(remaining) + " remaining");
    }
}

size_t EdgeGeometryCache::estimateMemoryUsage(const std::vector<gp_Pnt>& points) {
    // Estimate memory usage: vector overhead + point data
    // Each gp_Pnt has 3 doubles (8 bytes each) = 24 bytes
    // Vector has some overhead for size/capacity
    return sizeof(std::vector<gp_Pnt>) + points.capacity() * sizeof(gp_Pnt) + 32; // Add some buffer
}

bool EdgeGeometryCache::shouldEvictForNewEntry(size_t newEntrySize) {
    // Simple memory limit check - can be made configurable
    const size_t maxMemoryMB = 500; // 500MB limit
    const size_t maxMemoryBytes = maxMemoryMB * 1024 * 1024;

    return (m_totalMemoryUsage + newEntrySize) > maxMemoryBytes;
}

void EdgeGeometryCache::evictLRU() {
    if (m_cache.empty()) return;

    // Find least recently used entry
    auto lruIt = m_cache.begin();

    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->second.lastAccess < lruIt->second.lastAccess) {
            lruIt = it;
        }
    }

    if (lruIt != m_cache.end()) {
        size_t freedMemory = lruIt->second.memoryUsage;
        m_totalMemoryUsage -= freedMemory;
        std::string key = lruIt->first;
        m_cache.erase(lruIt);

        LOG_DBG_S("EdgeCache LRU evicted: " + key + " (" + std::to_string(freedMemory) + " bytes)");
    }
}




