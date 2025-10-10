#include "edges/EdgeGeometryCache.h"
#include "logger/Logger.h"
#include <sstream>

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
        
        CacheEntry entry;
        entry.points = points;
        entry.shapeHash = 0;
        entry.lastAccess = std::chrono::steady_clock::now();
        
        m_cache[key] = std::move(entry);
        cacheSize = m_cache.size();
        insertedNew = true;
    }
    
    // Log after releasing lock
    if (insertedNew) {
        LOG_INF_S("EdgeCache stored: " + key + 
                  " (" + std::to_string(points.size()) + " points, " +
                  "cache size: " + std::to_string(cacheSize) + " entries)");
    }
    
    return points;
}

void EdgeGeometryCache::invalidate(const std::string& key) {
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            m_cache.erase(it);
            found = true;
        }
    }
    
    if (found) {
        LOG_DBG_S("EdgeCache invalidated: " + key);
    }
}

void EdgeGeometryCache::clear() {
    size_t oldSize = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        oldSize = m_cache.size();
        m_cache.clear();
        m_hitCount = 0;
        m_missCount = 0;
    }
    
    LOG_INF_S("EdgeCache cleared (" + std::to_string(oldSize) + " entries removed)");
}

void EdgeGeometryCache::evictOldEntries(std::chrono::seconds maxAge) {
    size_t evicted = 0;
    size_t remaining = 0;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        
        for (auto it = m_cache.begin(); it != m_cache.end();) {
            if (now - it->second.lastAccess > maxAge) {
                it = m_cache.erase(it);
                evicted++;
            } else {
                ++it;
            }
        }
        
        remaining = m_cache.size();
    }
    
    if (evicted > 0) {
        LOG_INF_S("EdgeCache evicted " + std::to_string(evicted) + 
                  " old entries (remaining: " + std::to_string(remaining) + ")");
    }
}



