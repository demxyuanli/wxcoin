#pragma once

#include "STEPReader.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <future>

/**
 * @brief Fast STEP reader with optimized parsing and parallel processing
 * 
 * Optimizations include:
 * - Parallel entity processing
 * - Optimized string parsing
 * - Memory pre-allocation
 * - Batch shape operations
 */
class FastSTEPReader : public STEPReader {
public:
    /**
     * @brief Fast read configuration
     */
    struct FastReadConfig {
        bool enableParallelEntityProcessing = true;
        bool enableBatchShapeOperations = true;
        bool enableMemoryPreallocation = true;
        bool enableStringOptimization = true;
        int entityProcessingThreads = std::thread::hardware_concurrency();
        size_t batchSize = 100;
        size_t memoryPreallocationMB = 100;
    };

    /**
     * @brief Read STEP file with fast optimizations
     * @param filePath Path to STEP file
     * @param options Optimization options
     * @param config Fast read configuration
     * @param progress Progress callback
     * @return Read result
     */
    static ReadResult readSTEPFileFast(
        const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        const FastReadConfig& config = FastReadConfig(),
        ProgressCallback progress = nullptr
    );

private:
    /**
     * @brief Thread pool for parallel entity processing
     */
    class EntityProcessorPool {
    public:
        EntityProcessorPool(size_t numThreads);
        ~EntityProcessorPool();
        
        template<typename F>
        auto enqueue(F&& f) -> std::future<typename std::result_of<F()>::type>;
        
        void wait();
        
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queueMutex;
        std::condition_variable condition;
        std::condition_variable finished;
        std::atomic<bool> stop;
        std::atomic<size_t> activeTasks;
    };

    /**
     * @brief Optimized entity processor
     */
    class OptimizedEntityProcessor {
    public:
        OptimizedEntityProcessor(const FastReadConfig& config);
        
        void processEntities(
            const std::vector<Handle(Standard_Transient)>& entities,
            std::vector<TopoDS_Shape>& shapes
        );
        
    private:
        FastReadConfig m_config;
        std::unique_ptr<EntityProcessorPool> m_pool;
        
        TopoDS_Shape processEntity(const Handle(Standard_Transient)& entity);
        void processBatch(
            const std::vector<Handle(Standard_Transient)>& batch,
            std::vector<TopoDS_Shape>& results
        );
    };

    /**
     * @brief Memory pool for entity allocation
     */
    class EntityMemoryPool {
    public:
        EntityMemoryPool(size_t preallocMB);
        void* allocate(size_t size);
        void deallocate(void* ptr);
        void reset();
        
    private:
        std::vector<std::unique_ptr<uint8_t[]>> m_blocks;
        std::vector<size_t> m_blockSizes;
        std::mutex m_mutex;
        size_t m_currentBlock;
        size_t m_currentOffset;
    };

    /**
     * @brief String cache for repeated strings
     */
    class StringCache {
    public:
        const std::string& get(const char* str);
        void clear();
        
    private:
        std::unordered_map<std::string, std::string> m_cache;
        std::mutex m_mutex;
    };

    /**
     * @brief Batch shape operations
     */
    static std::vector<TopoDS_Shape> batchProcessShapes(
        const std::vector<TopoDS_Shape>& shapes,
        const OptimizationOptions& options
    );

    /**
     * @brief Optimize shape topology
     */
    static TopoDS_Shape optimizeShapeTopology(const TopoDS_Shape& shape);
};