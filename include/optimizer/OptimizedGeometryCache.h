#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"

// Forward declarations
struct GeometryKey;
struct CachedGeometry;

/**
 * @brief Thread pool for geometry computations
 */
class GeometryThreadPool {
public:
    GeometryThreadPool(size_t threadCount);
    ~GeometryThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) {
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_tasks.emplace([task]() { (*task)(); });
        }

        m_condition.notify_one();
        return res;
    }

    void shutdown();

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop;

    void workerFunction();
};

/**
 * @brief Geometry key for caching
 */
struct GeometryKey {
    std::string type;
    std::vector<double> params;
    size_t hash;

    GeometryKey(const std::string& t, const std::vector<double>& p);
    bool operator==(const GeometryKey& other) const;
};

/**
 * @brief Cached geometry data
 */
struct CachedGeometry {
    TopoDS_Shape shape;
    TriangleMesh mesh;
    std::chrono::steady_clock::time_point timestamp;
    size_t accessCount;
    uint32_t lastAccessTime;

    CachedGeometry();
    CachedGeometry(const TopoDS_Shape& s, const TriangleMesh& m);
};

// Hash function for GeometryKey
namespace std {
    template<> struct hash<GeometryKey> {
        size_t operator()(const GeometryKey& key) const {
            return key.hash;
        }
    };
}

/**
 * @brief Optimized geometry cache with thread pool support
 */
class OptimizedGeometryCache {
public:
    OptimizedGeometryCache(size_t maxCacheSize = 1000);
    ~OptimizedGeometryCache() = default;

    // Geometry retrieval and storage
    std::shared_ptr<CachedGeometry> getGeometry(const GeometryKey& key) const;
    std::shared_ptr<CachedGeometry> getOrCreateGeometry(const GeometryKey& key,
        const OCCMeshConverter::MeshParameters& meshParams,
        std::function<std::pair<TopoDS_Shape, TriangleMesh>()> creator);

    // Cache management
    void removeGeometry(const GeometryKey& key);
    void clearCache();
    void cleanupOldEntries(std::chrono::seconds maxAge);

    // Performance monitoring
    std::string getCacheStats() const;

    // Precomputation
    void precomputeGeometries(const std::vector<GeometryKey>& commonKeys);

private:
    mutable std::shared_mutex m_cacheMutex;
    std::unordered_map<GeometryKey, std::shared_ptr<CachedGeometry>> m_cache;
    size_t m_maxCacheSize;

public:
    // Thread pool for background computations
    GeometryThreadPool m_threadPool;

    // Performance tracking
    mutable std::atomic<uint64_t> m_cacheHits{0};
    mutable std::atomic<uint64_t> m_cacheMisses{0};
    mutable std::atomic<uint64_t> m_computations{0};

    // Internal methods
    void evictLeastUsed();
    void updateStats(bool hit) const;
};

/**
 * @brief Optimized shape builder with caching
 */
class OptimizedShapeBuilder {
public:
    OptimizedShapeBuilder();
    ~OptimizedShapeBuilder() = default;

    // Basic shape creation
    TopoDS_Shape createBox(double width, double height, double depth, const gp_Pnt& position = gp_Pnt(0, 0, 0));
    TopoDS_Shape createSphere(double radius, const gp_Pnt& center = gp_Pnt(0, 0, 0));
    TopoDS_Shape createCylinder(double radius, double height, const gp_Pnt& position = gp_Pnt(0, 0, 0));
    TopoDS_Shape createCone(double bottomRadius, double topRadius, double height, const gp_Pnt& position = gp_Pnt(0, 0, 0));
    TopoDS_Shape createTorus(double majorRadius, double minorRadius, const gp_Pnt& center = gp_Pnt(0, 0, 0));

    // Advanced shape operations
    TopoDS_Shape createExtrusion(const TopoDS_Shape& profile, const gp_Vec& direction);
    TopoDS_Shape createRevolution(const TopoDS_Shape& profile,
                                 const gp_Pnt& axisPosition,
                                 const gp_Dir& axisDirection,
                                 double angle);

    // Boolean operations
    TopoDS_Shape booleanUnion(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    TopoDS_Shape booleanIntersection(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    TopoDS_Shape booleanDifference(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);

    // Batch operations
    std::vector<TopoDS_Shape> createMultipleBoxes(
        const std::vector<std::tuple<double, double, double, gp_Pnt>>& params);
    std::vector<TopoDS_Shape> createMultipleSpheres(
        const std::vector<std::pair<double, gp_Pnt>>& params);

    // Cache management
    void clearCache();
    std::string getPerformanceStats() const;

private:
    std::unique_ptr<OptimizedGeometryCache> m_cache;

    // Internal helper methods
    GeometryKey createGeometryKey(const std::string& type, const std::vector<double>& params);
    TopoDS_Shape applyTransform(const TopoDS_Shape& shape, const gp_Pnt& position);
};
