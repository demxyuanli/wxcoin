#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <functional>
#include <optional>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>

/**
 * @brief Cache for edge geometry to avoid recomputation
 * 
 * Caches extracted edge points AND intersection points to significantly 
 * speed up edge display toggling and intersection detection.
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
    
    /**
     * @brief Edge-intersection relationship for incremental updates
     */
    struct EdgeIntersection {
        size_t edge1Index;      // Index in edge list
        size_t edge2Index;      // Index in edge list
        gp_Pnt intersectionPoint;
        double distance;         // Distance between edges at intersection
        
        EdgeIntersection() : edge1Index(0), edge2Index(0), distance(0.0) {}
        EdgeIntersection(size_t i1, size_t i2, const gp_Pnt& pt, double dist)
            : edge1Index(i1), edge2Index(i2), intersectionPoint(pt), distance(dist) {}
    };
    
    /**
     * @brief Intersection cache entry with additional metadata and edge relationships
     */
    struct IntersectionCacheEntry {
        std::vector<gp_Pnt> intersectionPoints;
        size_t shapeHash;
        double tolerance;
        std::chrono::steady_clock::time_point lastAccess;
        size_t memoryUsage;
        double computationTime;  // Track how long it took to compute
        
        // NEW: Edge-intersection relationships for incremental updates
        std::vector<EdgeIntersection> edgeIntersections;  // Which edges produce which intersections
        std::vector<size_t> edgeHashes;  // Hash of each edge for change detection
        
        IntersectionCacheEntry() : shapeHash(0), tolerance(0.0), memoryUsage(0), computationTime(0.0) {}
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
     * @brief Get cached intersections or compute if not cached
     * @param key Unique cache key (should include shape hash and tolerance)
     * @param computeFunc Function to compute intersection points if cache miss
     * @param shapeHash Hash of the shape for invalidation
     * @param tolerance Tolerance used for intersection detection
     * @return Cached or computed intersection points
     */
    std::vector<gp_Pnt> getOrComputeIntersections(
        const std::string& key,
        std::function<std::vector<gp_Pnt>()> computeFunc,
        size_t shapeHash,
        double tolerance);

    /**
     * @brief Try to get cached intersection points without computing
     * @param key Cache key
     * @return Optional vector of points (empty if not cached)
     */
    std::optional<std::vector<gp_Pnt>> tryGetCached(const std::string& key);
    
    /**
     * @brief Store intersection points in cache
     * @param key Cache key
     * @param points Points to cache
     * @param shapeHash Hash of the shape
     * @param tolerance Tolerance used
     */
    void storeCached(const std::string& key, const std::vector<gp_Pnt>& points,
                    size_t shapeHash, double tolerance);
    
    /**
     * @brief Invalidate specific cache entry
     * @param key Cache key to invalidate
     */
    void invalidate(const std::string& key);
    
    /**
     * @brief Invalidate intersection cache for a specific shape
     * @param shapeHash Hash of the shape to invalidate
     */
    void invalidateIntersections(size_t shapeHash);

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
    
    /**
     * @brief Result of incremental intersection update
     */
    struct IncrementalUpdateResult {
        std::vector<gp_Pnt> validIntersections;  // Still valid intersections
        std::vector<gp_Pnt> newIntersections;     // Need to compute
        std::vector<size_t> invalidatedEdgeIndices;  // Edges that changed
        
        IncrementalUpdateResult() {}
    };
    
    /**
     * @brief Compute hash for an edge (for change detection)
     * @param edge Edge to hash
     * @return Hash value
     */
    static size_t computeEdgeHash(const TopoDS_Edge& edge);
    
    /**
     * @brief Update intersections incrementally when geometry changes
     * @param key Cache key
     * @param currentEdges Current edge list
     * @param tolerance Tolerance for intersection detection
     * @param computeFunc Function to compute intersections for changed edges
     * @return Update result with valid and new intersections
     */
    IncrementalUpdateResult updateIntersectionsIncremental(
        const std::string& key,
        const std::vector<TopoDS_Edge>& currentEdges,
        double tolerance,
        std::function<std::vector<gp_Pnt>(const std::vector<size_t>&)> computeFunc);

private:
    EdgeGeometryCache() : m_hitCount(0), m_missCount(0), m_totalMemoryUsage(0), 
                          m_intersectionHitCount(0), m_intersectionMissCount(0) {}
    EdgeGeometryCache(const EdgeGeometryCache&) = delete;
    EdgeGeometryCache& operator=(const EdgeGeometryCache&) = delete;

    std::unordered_map<std::string, CacheEntry> m_cache;
    std::unordered_map<std::string, IntersectionCacheEntry> m_intersectionCache;
    mutable std::mutex m_mutex;
    size_t m_hitCount;
    size_t m_missCount;
    size_t m_totalMemoryUsage;
    size_t m_intersectionHitCount;
    size_t m_intersectionMissCount;
};



