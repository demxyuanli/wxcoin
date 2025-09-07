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
#include "CommandType.h"
#include "CommandListener.h"
#include "OCCMeshConverter.h"

// Forward declarations
class OCCGeometry;

namespace optimizer {

/**
 * @brief Optimized command dispatcher using integer IDs instead of strings
 */
class OptimizedCommandDispatcher {
public:
    using CommandId = uint32_t;
    using CommandParameters = std::unordered_map<std::string, std::string>;
    
    struct CommandResult {
        bool success;
        std::string message;
        CommandId commandId;
        
        CommandResult(bool s = true, const std::string& msg = "", CommandId id = 0)
            : success(s), message(msg), commandId(id) {}
    };

private:
    // Use integer IDs for O(1) lookup instead of string comparison
    std::unordered_map<CommandId, std::vector<std::shared_ptr<CommandListener>>> m_listeners;
    
    // Reverse mapping from hash ID to original command string
    std::unordered_map<CommandId, std::string> m_commandIdToString;
    
    // Parameter pool to reduce memory allocations
    struct ParameterPool {
        std::queue<CommandParameters*> m_available;
        std::vector<std::unique_ptr<CommandParameters>> m_pool;
        std::mutex m_mutex;
        
        CommandParameters* acquire() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_available.empty()) {
                m_pool.push_back(std::make_unique<CommandParameters>());
                m_available.push(m_pool.back().get());
            }
            CommandParameters* params = m_available.front();
            m_available.pop();
            params->clear();
            return params;
        }
        
        void release(CommandParameters* params) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_available.push(params);
        }
    };
    
    ParameterPool m_paramPool;
    std::function<void(const CommandResult&)> m_uiFeedbackHandler;
    mutable std::shared_mutex m_mutex;

public:
    OptimizedCommandDispatcher();
    ~OptimizedCommandDispatcher();
    
    // Register listener with integer ID
    void registerListener(CommandId commandId, std::shared_ptr<CommandListener> listener);
    
    // Register listener with command type enum (registers with both enum ID and string hash ID)
    void registerListener(cmd::CommandType commandType, std::shared_ptr<CommandListener> listener);
    
    // Unregister listener
    void unregisterListener(CommandId commandId, std::shared_ptr<CommandListener> listener);
    
    // Dispatch command with optimized integer lookup
    CommandResult dispatchCommand(CommandId commandId, const CommandParameters& parameters);
    
    // Check if handler exists
    bool hasHandler(CommandId commandId) const;
    
    // Set UI feedback handler
    void setUIFeedbackHandler(std::function<void(const CommandResult&)> handler);
    
    // Convert string command to ID (for backward compatibility)
    static CommandId stringToCommandId(const std::string& commandString);
    
    // Convert command type enum to ID
    static CommandId commandTypeToId(cmd::CommandType commandType);
};

/**
 * @brief Geometry computation cache with parallel processing support
 */
class GeometryComputationCache {
public:
    struct GeometryKey {
        std::string type;
        double params[6]; // Support up to 6 parameters
        size_t hash;
        
        GeometryKey(const std::string& t, const std::vector<double>& p);
        bool operator==(const GeometryKey& other) const;
    };
    
    struct CachedGeometry {
        TopoDS_Shape shape;
        std::chrono::steady_clock::time_point timestamp;
        size_t accessCount;
        
        CachedGeometry() 
            : timestamp(std::chrono::steady_clock::now()), accessCount(0) {}
        
        CachedGeometry(const TopoDS_Shape& s) 
            : shape(s), timestamp(std::chrono::steady_clock::now()), accessCount(1) {}
    };
    
    struct MeshCacheEntry {
        TriangleMesh mesh;
        std::chrono::steady_clock::time_point timestamp;
        size_t accessCount;
        
        MeshCacheEntry() 
            : timestamp(std::chrono::steady_clock::now()), accessCount(0) {}
        
        MeshCacheEntry(const TriangleMesh& m) 
            : mesh(m), timestamp(std::chrono::steady_clock::now()), accessCount(1) {}
    };

private:
    // Geometry shape cache
    std::unordered_map<size_t, CachedGeometry> m_geometryCache;
    mutable std::shared_mutex m_geometryMutex;
    
    // Mesh cache
    std::unordered_map<size_t, MeshCacheEntry> m_meshCache;
    mutable std::shared_mutex m_meshMutex;
    
    // Thread pool for parallel computation
    class ThreadPool {
    private:
        std::vector<std::thread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        std::mutex m_queueMutex;
        std::condition_variable m_condition;
        std::atomic<bool> m_stop{false};
        
    public:
        ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
        ~ThreadPool();
        
        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;
        
        void stop();
    };
    
    std::unique_ptr<ThreadPool> m_threadPool;
    
    // Cache management
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    static constexpr std::chrono::minutes CACHE_TTL{30};
    
    void cleanupExpiredEntries();
    size_t computeGeometryHash(const GeometryKey& key);
    size_t computeMeshHash(const TopoDS_Shape& shape, const OCCMeshConverter::MeshParameters& params);

public:
    GeometryComputationCache();
    ~GeometryComputationCache();
    
    // Get or create geometry shape with caching
    TopoDS_Shape getOrCreateGeometry(const GeometryKey& key, 
                                    std::function<TopoDS_Shape()> creator);
    
    // Get or create mesh with caching
    TriangleMesh getOrCreateMesh(const TopoDS_Shape& shape, 
                                const OCCMeshConverter::MeshParameters& params,
                                std::function<TriangleMesh()> creator);
    
    // Parallel geometry creation
    std::future<TopoDS_Shape> createGeometryAsync(const GeometryKey& key,
                                                 std::function<TopoDS_Shape()> creator);
    
    // Parallel mesh creation
    std::future<TriangleMesh> createMeshAsync(const TopoDS_Shape& shape,
                                             const OCCMeshConverter::MeshParameters& params,
                                             std::function<TriangleMesh()> creator);
    
    // Cache management
    void clearCache();
    void cleanup();
    size_t getCacheSize() const;
    size_t getMeshCacheSize() const;
};

/**
 * @brief Optimized container manager for geometry objects
 */
class OptimizedGeometryManager {
private:
    // Use unordered_map for O(1) lookup instead of vector
    std::unordered_map<std::string, std::shared_ptr<OCCGeometry>> m_geometryMap;
    
    // Separate containers for different access patterns
    std::vector<std::shared_ptr<OCCGeometry>> m_geometryList; // For iteration
    std::unordered_map<std::string, std::shared_ptr<OCCGeometry>> m_selectedGeometries;
    
    // Thread-safe access
    mutable std::shared_mutex m_mutex;
    
    // Performance metrics
    struct Metrics {
        std::atomic<size_t> lookupCount{0};
        std::atomic<size_t> cacheHits{0};
        std::atomic<size_t> cacheMisses{0};
        
        double getHitRate() const {
            size_t total = lookupCount.load();
            return total > 0 ? static_cast<double>(cacheHits.load()) / total : 0.0;
        }
        
        // Copy constructor for atomic types
        Metrics() = default;
        Metrics(const Metrics& other) 
            : lookupCount(other.lookupCount.load())
            , cacheHits(other.cacheHits.load())
            , cacheMisses(other.cacheMisses.load()) {}
        
        // Assignment operator
        Metrics& operator=(const Metrics& other) {
            if (this != &other) {
                lookupCount.store(other.lookupCount.load());
                cacheHits.store(other.cacheHits.load());
                cacheMisses.store(other.cacheMisses.load());
            }
            return *this;
        }
    };
    
    Metrics m_metrics;

public:
    OptimizedGeometryManager();
    ~OptimizedGeometryManager();
    
    // Optimized geometry management
    void addGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeGeometry(const std::string& name);
    void removeGeometry(std::shared_ptr<OCCGeometry> geometry);
    
    // Fast lookup methods
    std::shared_ptr<OCCGeometry> findGeometry(const std::string& name);
    std::vector<std::shared_ptr<OCCGeometry>> getAllGeometries() const;
    std::vector<std::shared_ptr<OCCGeometry>> getSelectedGeometries() const;
    
    // Selection management
    void selectGeometry(const std::string& name, bool selected = true);
    void selectAll();
    void deselectAll();
    
    // Batch operations for better performance
    void addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);
    void removeGeometries(const std::vector<std::string>& names);
    
    // Performance monitoring
    Metrics getMetrics() const { return m_metrics; }
    void resetMetrics();
    
    // Thread-safe iteration
    template<typename Func>
    void forEachGeometry(Func func) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        for (const auto& [name, geometry] : m_geometryMap) {
            func(name, geometry);
        }
    }
};

/**
 * @brief Main performance optimizer that coordinates all optimizations
 */
class PerformanceOptimizer {
private:
    std::unique_ptr<OptimizedCommandDispatcher> m_commandDispatcher;
    std::unique_ptr<GeometryComputationCache> m_geometryCache;
    std::unique_ptr<OptimizedGeometryManager> m_geometryManager;
    
    // Performance monitoring
    struct PerformanceMetrics {
        std::chrono::steady_clock::time_point startTime;
        std::unordered_map<std::string, std::chrono::nanoseconds> timings;
        std::mutex mutex;
        
        void recordTiming(const std::string& operation, std::chrono::nanoseconds duration);
        void printReport() const;
    };
    
    PerformanceMetrics m_metrics;
    
public:
    // Configuration
    struct Config {
        bool enableCommandOptimization = true;
        bool enableGeometryCaching = true;
        bool enableParallelProcessing = true;
        bool enableContainerOptimization = true;
        size_t threadPoolSize = std::thread::hardware_concurrency();
    };
private:
    Config m_config;

public:
    PerformanceOptimizer();
    ~PerformanceOptimizer();
    
    // Initialize optimizations
    void initialize(const Config& config = Config{});
    
    // Access to optimized components
    OptimizedCommandDispatcher* getCommandDispatcher() { return m_commandDispatcher.get(); }
    GeometryComputationCache* getGeometryCache() { return m_geometryCache.get(); }
    OptimizedGeometryManager* getGeometryManager() { return m_geometryManager.get(); }
    
    // Performance monitoring
    void startTiming(const std::string& operation);
    void endTiming(const std::string& operation);
    void printPerformanceReport() const;
    
    // Configuration
    void setConfig(const Config& config);
    const Config& getConfig() const { return m_config; }
    
    // Cleanup
    void cleanup();
};

} // namespace optimizer

// Global performance optimizer instance
extern std::unique_ptr<optimizer::PerformanceOptimizer> g_performanceOptimizer;

// Global cleanup function
void cleanupGlobalOptimizer();

// Convenience macros for performance monitoring
#define PERFORMANCE_TIMING(operation) \
    auto timing_start_##operation = std::chrono::steady_clock::now(); \
    auto timing_guard_##operation = [&]() { \
        auto duration = std::chrono::steady_clock::now() - timing_start_##operation; \
        if (g_performanceOptimizer) { \
            g_performanceOptimizer->endTiming(#operation); \
        } \
    }; \
    std::unique_ptr<void, decltype(timing_guard_##operation)> timing_scope_##operation(nullptr, timing_guard_##operation)

#define START_PERFORMANCE_TIMING(operation) \
    if (g_performanceOptimizer) { \
        g_performanceOptimizer->startTiming(#operation); \
    }

#define END_PERFORMANCE_TIMING(operation) \
    if (g_performanceOptimizer) { \
        g_performanceOptimizer->endTiming(#operation); \
    } 