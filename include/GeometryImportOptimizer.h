#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>
#include <future>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include "GeometryReader.h"

/**
 * @brief Advanced optimization system for geometry imports
 * 
 * Provides multi-threaded processing, caching, and performance monitoring
 * for efficient geometry file imports
 */
class GeometryImportOptimizer {
public:
    /**
     * @brief Import performance metrics
     */
    struct ImportMetrics {
        size_t fileSize = 0;
        double readTime = 0.0;      // File reading time (ms)
        double parseTime = 0.0;     // Parsing time (ms)
        double tessellationTime = 0.0; // Tessellation time (ms)
        double totalTime = 0.0;     // Total import time (ms)
        size_t geometryCount = 0;
        size_t triangleCount = 0;
        size_t memoryUsed = 0;      // Memory usage in bytes
        bool usedCache = false;
        int threadCount = 1;
    };

    /**
     * @brief Cache entry for imported geometries
     */
    struct CacheEntry {
        std::vector<std::shared_ptr<OCCGeometry>> geometries;
        std::chrono::system_clock::time_point timestamp;
        size_t fileHash;
        ImportMetrics metrics;
    };

    /**
     * @brief Multi-threaded import configuration
     */
    struct ThreadedImportConfig {
        int maxThreads = std::thread::hardware_concurrency();
        bool enableParallelReading = true;
        bool enableParallelParsing = true;
        bool enableParallelTessellation = true;
        size_t chunkSize = 1024 * 1024; // 1MB chunks for parallel reading
        bool useMemoryMapping = true;
    };

    /**
     * @brief Progressive loading configuration
     */
    struct ProgressiveLoadConfig {
        bool enabled = true;
        double lodDistances[4] = { 10.0, 50.0, 100.0, 500.0 };
        double lodDeflections[4] = { 0.1, 0.5, 1.0, 2.0 };
        bool streamLargeFiles = true;
        size_t streamThreshold = 50 * 1024 * 1024; // 50MB
    };

    /**
     * @brief Enhanced optimization options
     */
    struct EnhancedOptions : public GeometryReader::OptimizationOptions {
        ThreadedImportConfig threading;
        ProgressiveLoadConfig progressive;
        bool enableCache = true;
        size_t maxCacheSize = 1024 * 1024 * 1024; // 1GB
        bool enableGPUAcceleration = false;
        bool enablePrefetch = true;
        bool enableCompression = true;
    };

    /**
     * @brief Import geometry file with advanced optimizations
     * @param filePath Path to the geometry file
     * @param options Enhanced optimization options
     * @param progress Progress callback
     * @return Import result with metrics
     */
    static GeometryReader::ReadResult importOptimized(
        const std::string& filePath,
        const EnhancedOptions& options = EnhancedOptions(),
        GeometryReader::ProgressCallback progress = nullptr
    );

    /**
     * @brief Import multiple files in parallel
     * @param filePaths Vector of file paths
     * @param options Enhanced optimization options
     * @param progress Progress callback
     * @return Vector of import results
     */
    static std::vector<GeometryReader::ReadResult> importBatchOptimized(
        const std::vector<std::string>& filePaths,
        const EnhancedOptions& options = EnhancedOptions(),
        std::function<void(size_t, size_t, const std::string&)> progress = nullptr
    );

    /**
     * @brief Get cached import result if available
     * @param filePath Path to the geometry file
     * @return Cached result or empty result
     */
    static std::optional<CacheEntry> getCachedImport(const std::string& filePath);

    /**
     * @brief Clear import cache
     */
    static void clearCache();

    /**
     * @brief Get cache statistics
     * @return Cache usage information
     */
    static std::string getCacheStatistics();

    /**
     * @brief Enable/disable performance profiling
     * @param enable True to enable profiling
     */
    static void enableProfiling(bool enable);

    /**
     * @brief Get performance report
     * @return Performance statistics as string
     */
    static std::string getPerformanceReport();

    /**
     * @brief Preload file into memory for faster access
     * @param filePath Path to preload
     * @return True if successful
     */
    static bool preloadFile(const std::string& filePath);

    /**
     * @brief Estimate import time for a file
     * @param filePath Path to the file
     * @return Estimated time in milliseconds
     */
    static double estimateImportTime(const std::string& filePath);

private:
    // Cache management
    static std::unordered_map<std::string, CacheEntry> s_cache;
    static std::mutex s_cacheMutex;
    static std::atomic<size_t> s_cacheSize;
    static const size_t s_maxCacheSize;

    // Performance tracking
    static std::vector<ImportMetrics> s_performanceHistory;
    static std::mutex s_performanceMutex;
    static std::atomic<bool> s_profilingEnabled;

    // Memory pool for efficient allocation
    static std::unique_ptr<class MemoryPool> s_memoryPool;

    /**
     * @brief Calculate file hash for caching
     * @param filePath Path to the file
     * @return Hash value
     */
    static size_t calculateFileHash(const std::string& filePath);

    /**
     * @brief Import with multi-threading
     * @param reader Geometry reader instance
     * @param filePath File path
     * @param options Options
     * @param progress Progress callback
     * @return Import result
     */
    static GeometryReader::ReadResult importWithThreading(
        std::unique_ptr<GeometryReader> reader,
        const std::string& filePath,
        const EnhancedOptions& options,
        GeometryReader::ProgressCallback progress
    );

    /**
     * @brief Apply progressive loading
     * @param geometries Geometries to process
     * @param options Progressive loading options
     */
    static void applyProgressiveLoading(
        std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        const ProgressiveLoadConfig& options
    );

    /**
     * @brief Optimize memory usage during import
     * @param geometries Imported geometries
     */
    static void optimizeMemoryUsage(
        std::vector<std::shared_ptr<OCCGeometry>>& geometries
    );
};

/**
 * @brief Memory pool for efficient geometry allocation
 */
class MemoryPool {
public:
    MemoryPool(size_t blockSize = 1024 * 1024); // 1MB blocks
    ~MemoryPool();

    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    void reset();

    size_t getTotalAllocated() const { return m_totalAllocated; }
    size_t getUsedMemory() const { return m_usedMemory; }

private:
    struct Block {
        std::unique_ptr<uint8_t[]> memory;
        size_t size;
        size_t used;
    };

    std::vector<Block> m_blocks;
    size_t m_blockSize;
    std::atomic<size_t> m_totalAllocated;
    std::atomic<size_t> m_usedMemory;
    std::mutex m_mutex;
};