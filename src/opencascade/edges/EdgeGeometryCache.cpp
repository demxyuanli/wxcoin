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

// Intersection cache implementation
std::vector<gp_Pnt> EdgeGeometryCache::getOrComputeIntersections(
    const std::string& key,
    std::function<std::vector<gp_Pnt>()> computeFunc,
    size_t shapeHash,
    double tolerance)
{
    // Check cache first
    bool cacheHit = false;
    std::vector<gp_Pnt> cachedPoints;
    size_t pointsSize = 0;
    double cachedComputationTime = 0.0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_intersectionCache.find(key);
        if (it != m_intersectionCache.end()) {
            // Verify tolerance matches (important for precision)
            if (std::abs(it->second.tolerance - tolerance) < 1e-9) {
                it->second.lastAccess = std::chrono::steady_clock::now();
                m_intersectionHitCount++;
                cacheHit = true;
                cachedPoints = it->second.intersectionPoints;
                pointsSize = cachedPoints.size();
                cachedComputationTime = it->second.computationTime;
            }
            else {
                // Tolerance mismatch - invalidate and recompute
                LOG_DBG_S("IntersectionCache tolerance mismatch for " + key + 
                         ", recomputing (cached: " + std::to_string(it->second.tolerance) +
                         ", requested: " + std::to_string(tolerance) + ")");
                m_totalMemoryUsage -= it->second.memoryUsage;
                m_intersectionCache.erase(it);
                m_intersectionMissCount++;
            }
        }
        else {
            m_intersectionMissCount++;
        }
    }

    if (cacheHit) {
        LOG_INF_S("IntersectionCache HIT: " + key + " (" + std::to_string(pointsSize) + 
                  " points, saved " + std::to_string(cachedComputationTime) + "s computation)");
        return cachedPoints;
    }

    LOG_INF_S("IntersectionCache MISS: " + key + " (computing...)");

    // Compute with timing
    auto startTime = std::chrono::high_resolution_clock::now();
    auto points = computeFunc();
    auto endTime = std::chrono::high_resolution_clock::now();
    double computationTime = std::chrono::duration<double>(endTime - startTime).count();

    // Cache the result
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Double-check
        auto it = m_intersectionCache.find(key);
        if (it != m_intersectionCache.end()) {
            return it->second.intersectionPoints;
        }

        IntersectionCacheEntry entry;
        entry.intersectionPoints = points;
        entry.shapeHash = shapeHash;
        entry.tolerance = tolerance;
        entry.lastAccess = std::chrono::steady_clock::now();
        entry.memoryUsage = estimateMemoryUsage(points);
        entry.computationTime = computationTime;

        m_intersectionCache[key] = std::move(entry);
        m_totalMemoryUsage += entry.memoryUsage;
        
        LOG_INF_S("IntersectionCache stored: " + key + " (" + std::to_string(points.size()) +
                  " points, " + std::to_string(entry.memoryUsage) + " bytes, " +
                  std::to_string(computationTime) + "s)");
    }

    return points;
}

std::optional<std::vector<gp_Pnt>> EdgeGeometryCache::tryGetCached(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_intersectionCache.find(key);
    if (it != m_intersectionCache.end()) {
        m_intersectionHitCount++;
        LOG_INF_S("IntersectionCache HIT: " + key);
        return it->second.intersectionPoints;
    }
    
    m_intersectionMissCount++;
    return std::nullopt;
}

void EdgeGeometryCache::storeCached(const std::string& key, const std::vector<gp_Pnt>& points,
                                   size_t shapeHash, double tolerance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    IntersectionCacheEntry entry;
    entry.intersectionPoints = points;
    entry.shapeHash = shapeHash;
    entry.tolerance = tolerance;
    entry.computationTime = 0.0;
    entry.memoryUsage = estimateMemoryUsage(points);
    
    m_intersectionCache[key] = entry;
    m_totalMemoryUsage += entry.memoryUsage;
    
    LOG_INF_S("IntersectionCache STORED: " + key + " (" + std::to_string(points.size()) + " points)");
}

void EdgeGeometryCache::invalidateIntersections(size_t shapeHash) {
    size_t removedCount = 0;
    size_t freedMemory = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto it = m_intersectionCache.begin(); it != m_intersectionCache.end();) {
            if (it->second.shapeHash == shapeHash) {
                freedMemory += it->second.memoryUsage;
                m_totalMemoryUsage -= it->second.memoryUsage;
                it = m_intersectionCache.erase(it);
                removedCount++;
            }
            else {
                ++it;
            }
        }
    }

    if (removedCount > 0) {
        LOG_INF_S("IntersectionCache invalidated " + std::to_string(removedCount) +
                  " entries for shape (freed " + std::to_string(freedMemory) + " bytes)");
    }
}


