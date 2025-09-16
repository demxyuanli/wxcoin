#include "GeometryImportOptimizer.h"
#include "GeometryReader.h"
#include "logger/Logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <thread>
#include <future>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// Static member initialization
std::unordered_map<std::string, GeometryImportOptimizer::CacheEntry> GeometryImportOptimizer::s_cache;
std::mutex GeometryImportOptimizer::s_cacheMutex;
std::atomic<size_t> GeometryImportOptimizer::s_cacheSize(0);
const size_t GeometryImportOptimizer::s_maxCacheSize = 1024 * 1024 * 1024; // 1GB

std::vector<GeometryImportOptimizer::ImportMetrics> GeometryImportOptimizer::s_performanceHistory;
std::mutex GeometryImportOptimizer::s_performanceMutex;
std::atomic<bool> GeometryImportOptimizer::s_profilingEnabled(false);

std::unique_ptr<MemoryPool> GeometryImportOptimizer::s_memoryPool = std::make_unique<MemoryPool>();

// Memory-mapped file reader for faster I/O
class MemoryMappedFile {
public:
    MemoryMappedFile(const std::string& filePath) : m_filePath(filePath), m_data(nullptr), m_size(0) {
#ifdef _WIN32
        m_fileHandle = CreateFileA(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, 
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_fileHandle == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(m_fileHandle, &fileSize)) {
            CloseHandle(m_fileHandle);
            throw std::runtime_error("Failed to get file size");
        }
        m_size = static_cast<size_t>(fileSize.QuadPart);

        m_mapHandle = CreateFileMappingA(m_fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (m_mapHandle == nullptr) {
            CloseHandle(m_fileHandle);
            throw std::runtime_error("Failed to create file mapping");
        }

        m_data = MapViewOfFile(m_mapHandle, FILE_MAP_READ, 0, 0, 0);
        if (m_data == nullptr) {
            CloseHandle(m_mapHandle);
            CloseHandle(m_fileHandle);
            throw std::runtime_error("Failed to map view of file");
        }
#else
        m_fd = open(filePath.c_str(), O_RDONLY);
        if (m_fd == -1) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        struct stat st;
        if (fstat(m_fd, &st) == -1) {
            close(m_fd);
            throw std::runtime_error("Failed to get file size");
        }
        m_size = st.st_size;

        m_data = mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
        if (m_data == MAP_FAILED) {
            close(m_fd);
            throw std::runtime_error("Failed to map file");
        }
#endif
    }

    ~MemoryMappedFile() {
#ifdef _WIN32
        if (m_data) UnmapViewOfFile(m_data);
        if (m_mapHandle) CloseHandle(m_mapHandle);
        if (m_fileHandle != INVALID_HANDLE_VALUE) CloseHandle(m_fileHandle);
#else
        if (m_data && m_data != MAP_FAILED) munmap(m_data, m_size);
        if (m_fd != -1) close(m_fd);
#endif
    }

    const void* data() const { return m_data; }
    size_t size() const { return m_size; }

private:
    std::string m_filePath;
    void* m_data;
    size_t m_size;
#ifdef _WIN32
    HANDLE m_fileHandle = INVALID_HANDLE_VALUE;
    HANDLE m_mapHandle = nullptr;
#else
    int m_fd = -1;
#endif
};

GeometryReader::ReadResult GeometryImportOptimizer::importOptimized(
    const std::string& filePath,
    const EnhancedOptions& options,
    GeometryReader::ProgressCallback progress)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    ImportMetrics metrics;
    metrics.fileSize = std::filesystem::file_size(filePath);

    try {
        // Check cache first
        if (options.enableCache) {
            auto cached = getCachedImport(filePath);
            if (cached.has_value()) {
                LOG_INF_S("Using cached import for: " + filePath);
                GeometryReader::ReadResult result;
                result.success = true;
                result.geometries = cached->geometries;
                result.importTime = 0.0; // Instant from cache
                
                if (s_profilingEnabled) {
                    metrics.usedCache = true;
                    metrics.totalTime = 0.0;
                    std::lock_guard<std::mutex> lock(s_performanceMutex);
                    s_performanceHistory.push_back(metrics);
                }
                
                return result;
            }
        }

        // Prefetch file data
        if (options.enablePrefetch && metrics.fileSize < 100 * 1024 * 1024) { // < 100MB
            preloadFile(filePath);
        }

        // Get appropriate reader
        auto reader = GeometryReaderFactory::getReaderForFile(filePath);
        if (!reader) {
            GeometryReader::ReadResult result;
            result.success = false;
            result.errorMessage = "No reader available for file: " + filePath;
            return result;
        }

        // Import with threading optimizations
        auto result = importWithThreading(std::move(reader), filePath, options, progress);

        // Apply progressive loading if enabled
        if (options.progressive.enabled && !result.geometries.empty()) {
            applyProgressiveLoading(result.geometries, options.progressive);
        }

        // Optimize memory usage
        optimizeMemoryUsage(result.geometries);

        // Update cache
        if (options.enableCache && result.success) {
            CacheEntry entry;
            entry.geometries = result.geometries;
            entry.timestamp = std::chrono::system_clock::now();
            entry.fileHash = calculateFileHash(filePath);
            entry.metrics = metrics;

            std::lock_guard<std::mutex> lock(s_cacheMutex);
            // Check cache size limit
            if (s_cacheSize + metrics.memoryUsed < s_maxCacheSize) {
                s_cache[filePath] = entry;
                s_cacheSize += metrics.memoryUsed;
            }
        }

        // Record metrics
        auto endTime = std::chrono::high_resolution_clock::now();
        metrics.totalTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        metrics.geometryCount = result.geometries.size();
        
        if (s_profilingEnabled) {
            std::lock_guard<std::mutex> lock(s_performanceMutex);
            s_performanceHistory.push_back(metrics);
        }

        result.importTime = metrics.totalTime;
        return result;

    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in optimized import: " + std::string(e.what()));
        GeometryReader::ReadResult result;
        result.success = false;
        result.errorMessage = e.what();
        return result;
    }
}

std::vector<GeometryReader::ReadResult> GeometryImportOptimizer::importBatchOptimized(
    const std::vector<std::string>& filePaths,
    const EnhancedOptions& options,
    std::function<void(size_t, size_t, const std::string&)> progress)
{
    const size_t fileCount = filePaths.size();
    std::vector<GeometryReader::ReadResult> results(fileCount);
    
    // Determine optimal thread count
    const size_t threadCount = std::min(
        static_cast<size_t>(options.threading.maxThreads),
        fileCount
    );
    
    LOG_INF_S("Batch importing " + std::to_string(fileCount) + " files using " + 
              std::to_string(threadCount) + " threads");

    // Use thread pool for parallel imports
    std::vector<std::future<GeometryReader::ReadResult>> futures;
    futures.reserve(fileCount);

    // Launch async tasks
    for (size_t i = 0; i < fileCount; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            auto result = importOptimized(filePaths[i], options, nullptr);
            if (progress) {
                progress(i + 1, fileCount, filePaths[i]);
            }
            return result;
        }));
    }

    // Collect results
    for (size_t i = 0; i < fileCount; ++i) {
        results[i] = futures[i].get();
    }

    return results;
}

std::optional<GeometryImportOptimizer::CacheEntry> GeometryImportOptimizer::getCachedImport(
    const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    auto it = s_cache.find(filePath);
    if (it != s_cache.end()) {
        // Verify file hasn't changed
        size_t currentHash = calculateFileHash(filePath);
        if (currentHash == it->second.fileHash) {
            return it->second;
        } else {
            // File changed, invalidate cache
            s_cacheSize -= it->second.metrics.memoryUsed;
            s_cache.erase(it);
        }
    }
    return std::nullopt;
}

void GeometryImportOptimizer::clearCache()
{
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    s_cache.clear();
    s_cacheSize = 0;
    LOG_INF_S("Geometry import cache cleared");
}

std::string GeometryImportOptimizer::getCacheStatistics()
{
    std::lock_guard<std::mutex> lock(s_cacheMutex);
    std::ostringstream oss;
    oss << "Cache Statistics:\n";
    oss << "  Entries: " << s_cache.size() << "\n";
    oss << "  Size: " << (s_cacheSize / (1024.0 * 1024.0)) << " MB\n";
    oss << "  Max Size: " << (s_maxCacheSize / (1024.0 * 1024.0)) << " MB\n";
    oss << "  Usage: " << (100.0 * s_cacheSize / s_maxCacheSize) << "%\n";
    
    if (!s_cache.empty()) {
        oss << "\nCached Files:\n";
        for (const auto& [path, entry] : s_cache) {
            oss << "  " << std::filesystem::path(path).filename().string() 
                << " (" << entry.geometries.size() << " geometries, "
                << (entry.metrics.memoryUsed / 1024.0) << " KB)\n";
        }
    }
    
    return oss.str();
}

void GeometryImportOptimizer::enableProfiling(bool enable)
{
    s_profilingEnabled = enable;
    if (enable) {
        LOG_INF_S("Geometry import profiling enabled");
    } else {
        LOG_INF_S("Geometry import profiling disabled");
    }
}

std::string GeometryImportOptimizer::getPerformanceReport()
{
    std::lock_guard<std::mutex> lock(s_performanceMutex);
    
    if (s_performanceHistory.empty()) {
        return "No performance data available";
    }
    
    std::ostringstream oss;
    oss << "Import Performance Report\n";
    oss << "========================\n\n";
    
    // Calculate statistics
    double totalTime = 0.0;
    size_t totalFiles = s_performanceHistory.size();
    size_t cachedImports = 0;
    double avgReadTime = 0.0;
    double avgParseTime = 0.0;
    double avgTessellationTime = 0.0;
    
    for (const auto& metrics : s_performanceHistory) {
        totalTime += metrics.totalTime;
        if (metrics.usedCache) cachedImports++;
        avgReadTime += metrics.readTime;
        avgParseTime += metrics.parseTime;
        avgTessellationTime += metrics.tessellationTime;
    }
    
    avgReadTime /= totalFiles;
    avgParseTime /= totalFiles;
    avgTessellationTime /= totalFiles;
    
    oss << "Total Imports: " << totalFiles << "\n";
    oss << "Cached Imports: " << cachedImports << " (" 
        << (100.0 * cachedImports / totalFiles) << "%)\n";
    oss << "Total Time: " << totalTime << " ms\n";
    oss << "Average Times:\n";
    oss << "  Read: " << avgReadTime << " ms\n";
    oss << "  Parse: " << avgParseTime << " ms\n";
    oss << "  Tessellation: " << avgTessellationTime << " ms\n";
    oss << "  Total: " << (totalTime / totalFiles) << " ms\n";
    
    // Memory statistics
    size_t totalMemory = 0;
    for (const auto& metrics : s_performanceHistory) {
        totalMemory += metrics.memoryUsed;
    }
    oss << "\nMemory Usage:\n";
    oss << "  Total: " << (totalMemory / (1024.0 * 1024.0)) << " MB\n";
    oss << "  Average: " << (totalMemory / totalFiles / 1024.0) << " KB per import\n";
    
    // Thread utilization
    std::map<int, int> threadCounts;
    for (const auto& metrics : s_performanceHistory) {
        threadCounts[metrics.threadCount]++;
    }
    oss << "\nThread Utilization:\n";
    for (const auto& [threads, count] : threadCounts) {
        oss << "  " << threads << " threads: " << count << " imports\n";
    }
    
    return oss.str();
}

bool GeometryImportOptimizer::preloadFile(const std::string& filePath)
{
    try {
        // Use memory-mapped file for preloading
        MemoryMappedFile mmf(filePath);
        
        // Touch pages to load into memory
        const char* data = static_cast<const char*>(mmf.data());
        size_t pageSize = 4096;
        volatile char dummy = 0;
        for (size_t i = 0; i < mmf.size(); i += pageSize) {
            dummy += data[i];
        }
        
        LOG_INF_S("Preloaded file: " + filePath + " (" + 
                  std::to_string(mmf.size() / 1024) + " KB)");
        return true;
    } catch (const std::exception& e) {
        LOG_WRN_S("Failed to preload file: " + std::string(e.what()));
        return false;
    }
}

double GeometryImportOptimizer::estimateImportTime(const std::string& filePath)
{
    try {
        size_t fileSize = std::filesystem::file_size(filePath);
        std::string extension = std::filesystem::path(filePath).extension().string();
        
        // Base estimates (ms per MB)
        double timePerMB = 100.0; // Default
        
        // Adjust based on format
        if (extension == ".step" || extension == ".stp") {
            timePerMB = 150.0; // STEP files are complex
        } else if (extension == ".stl") {
            timePerMB = 50.0; // STL is simple
        } else if (extension == ".obj") {
            timePerMB = 75.0; // OBJ is moderate
        }
        
        // Adjust based on historical data
        std::lock_guard<std::mutex> lock(s_performanceMutex);
        if (!s_performanceHistory.empty()) {
            double avgTimePerByte = 0.0;
            int count = 0;
            for (const auto& metrics : s_performanceHistory) {
                if (metrics.fileSize > 0 && !metrics.usedCache) {
                    avgTimePerByte += metrics.totalTime / metrics.fileSize;
                    count++;
                }
            }
            if (count > 0) {
                timePerMB = (avgTimePerByte / count) * 1024 * 1024;
            }
        }
        
        return (fileSize / (1024.0 * 1024.0)) * timePerMB;
        
    } catch (const std::exception& e) {
        LOG_WRN_S("Failed to estimate import time: " + std::string(e.what()));
        return -1.0;
    }
}

size_t GeometryImportOptimizer::calculateFileHash(const std::string& filePath)
{
    try {
        // Simple hash based on file size and modification time
        auto fileSize = std::filesystem::file_size(filePath);
        auto modTime = std::filesystem::last_write_time(filePath);
        
        size_t hash = std::hash<size_t>{}(fileSize);
        hash ^= std::hash<decltype(modTime)::rep>{}(modTime.time_since_epoch().count());
        
        return hash;
    } catch (const std::exception& e) {
        LOG_WRN_S("Failed to calculate file hash: " + std::string(e.what()));
        return 0;
    }
}

GeometryReader::ReadResult GeometryImportOptimizer::importWithThreading(
    std::unique_ptr<GeometryReader> reader,
    const std::string& filePath,
    const EnhancedOptions& options,
    GeometryReader::ProgressCallback progress)
{
    auto metrics = std::make_unique<ImportMetrics>();
    auto readStart = std::chrono::high_resolution_clock::now();
    
    // Create options for the reader
    GeometryReader::OptimizationOptions readerOptions;
    readerOptions.enableParallelProcessing = options.threading.enableParallelParsing;
    readerOptions.enableShapeAnalysis = options.enableShapeAnalysis;
    readerOptions.enableCaching = options.enableCaching;
    readerOptions.enableBatchOperations = options.enableBatchOperations;
    readerOptions.enableNormalProcessing = options.enableNormalProcessing;
    readerOptions.maxThreads = options.threading.maxThreads;
    readerOptions.precision = options.precision;
    readerOptions.meshDeflection = options.meshDeflection;
    readerOptions.angularDeflection = options.angularDeflection;
    readerOptions.enableFineTessellation = options.enableFineTessellation;
    readerOptions.tessellationDeflection = options.tessellationDeflection;
    readerOptions.tessellationAngle = options.tessellationAngle;
    readerOptions.tessellationMinPoints = options.tessellationMinPoints;
    readerOptions.tessellationMaxPoints = options.tessellationMaxPoints;
    readerOptions.enableAdaptiveTessellation = options.enableAdaptiveTessellation;
    readerOptions.decomposition = options.decomposition;
    
    // Use memory-mapped file for large files if enabled
    if (options.threading.useMemoryMapping && 
        metrics->fileSize > 10 * 1024 * 1024) { // > 10MB
        try {
            LOG_INF_S("Using memory-mapped I/O for: " + filePath);
            // Note: Individual readers would need to be modified to accept memory-mapped data
            // For now, we'll use the standard file reading
        } catch (const std::exception& e) {
            LOG_WRN_S("Memory mapping failed, using standard I/O: " + std::string(e.what()));
        }
    }
    
    // Read file with the reader
    auto result = reader->readFile(filePath, readerOptions, progress);
    
    auto readEnd = std::chrono::high_resolution_clock::now();
    metrics->readTime = std::chrono::duration<double, std::milli>(readEnd - readStart).count();
    metrics->threadCount = options.threading.maxThreads;
    
    return result;
}

void GeometryImportOptimizer::applyProgressiveLoading(
    std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    const ProgressiveLoadConfig& options)
{
    LOG_INF_S("Applying progressive loading to " + std::to_string(geometries.size()) + " geometries");
    
    // For each geometry, create LOD versions
    for (auto& geometry : geometries) {
        if (!geometry || geometry->getShape().IsNull()) continue;
        
        // Store original shape as highest quality LOD
        geometry->setEnableLOD(true);
        
        // Generate lower quality LODs based on distance thresholds
        for (int i = 0; i < 4; ++i) {
            geometry->addLODLevel(options.lodDistances[i], options.lodDeflections[i]);
        }
    }
}

void GeometryImportOptimizer::optimizeMemoryUsage(
    std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    // Compact memory allocations
    for (auto& geometry : geometries) {
        if (!geometry) continue;
        
        // Release any temporary data
        geometry->releaseTemporaryData();
        
        // Optimize internal structures
        geometry->optimizeMemory();
    }
    
    // Shrink vectors to exact size
    geometries.shrink_to_fit();
}

// MemoryPool implementation
MemoryPool::MemoryPool(size_t blockSize) 
    : m_blockSize(blockSize), m_totalAllocated(0), m_usedMemory(0) {
}

MemoryPool::~MemoryPool() {
    reset();
}

void* MemoryPool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find a block with enough space
    for (auto& block : m_blocks) {
        if (block.size - block.used >= size) {
            void* ptr = block.memory.get() + block.used;
            block.used += size;
            m_usedMemory += size;
            return ptr;
        }
    }
    
    // Allocate new block
    size_t newBlockSize = std::max(size, m_blockSize);
    Block newBlock;
    newBlock.memory = std::make_unique<uint8_t[]>(newBlockSize);
    newBlock.size = newBlockSize;
    newBlock.used = size;
    
    void* ptr = newBlock.memory.get();
    m_blocks.push_back(std::move(newBlock));
    
    m_totalAllocated += newBlockSize;
    m_usedMemory += size;
    
    return ptr;
}

void MemoryPool::deallocate(void* ptr, size_t size) {
    // Memory pool doesn't support individual deallocation
    // Memory is freed when the pool is reset or destroyed
}

void MemoryPool::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocks.clear();
    m_totalAllocated = 0;
    m_usedMemory = 0;
}