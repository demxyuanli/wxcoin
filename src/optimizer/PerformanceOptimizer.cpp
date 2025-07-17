#include "optimizer/PerformanceOptimizer.h"
#include "logger/Logger.h"
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "OCCShapeBuilder.h"
#include <algorithm>
#include <iostream>
#include <condition_variable>
#include <functional>

// Global performance optimizer instance
std::unique_ptr<optimizer::PerformanceOptimizer> g_performanceOptimizer;

// Global cleanup function to ensure proper shutdown
void cleanupGlobalOptimizer() {
    if (g_performanceOptimizer) {
        g_performanceOptimizer->cleanup();
        g_performanceOptimizer.reset();
    }
}

namespace optimizer {

// ============================================================================
// OptimizedCommandDispatcher Implementation
// ============================================================================

OptimizedCommandDispatcher::OptimizedCommandDispatcher() {
    LOG_INF_S("OptimizedCommandDispatcher initialized");
}

OptimizedCommandDispatcher::~OptimizedCommandDispatcher() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_listeners.clear();
    LOG_INF_S("OptimizedCommandDispatcher destroyed");
}

void OptimizedCommandDispatcher::registerListener(CommandId commandId, std::shared_ptr<CommandListener> listener) {
    if (!listener) {
        LOG_ERR_S("Attempted to register null listener for command ID: " + std::to_string(commandId));
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_listeners[commandId].push_back(listener);
    LOG_INF_S("Registered listener '" + listener->getListenerName() + "' for command ID: " + std::to_string(commandId));
}

void OptimizedCommandDispatcher::registerListener(cmd::CommandType commandType, std::shared_ptr<CommandListener> listener) {
    // Register with both enum ID and string hash ID for compatibility
    CommandId enumId = static_cast<CommandId>(commandType);
    std::string commandString = cmd::to_string(commandType);
    CommandId stringId = stringToCommandId(commandString);
    
    if (!listener) {
        LOG_ERR_S("Attempted to register null listener for command type: " + commandString);
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    // Register with enum ID
    m_listeners[enumId].push_back(listener);
    m_commandIdToString[enumId] = commandString;
    LOG_INF_S("Registered listener '" + listener->getListenerName() + "' for command enum ID: " + std::to_string(enumId));
    
    // Register with string hash ID if different from enum ID
    if (enumId != stringId) {
        m_listeners[stringId].push_back(listener);
        m_commandIdToString[stringId] = commandString;
        LOG_INF_S("Registered listener '" + listener->getListenerName() + "' for command string ID: " + std::to_string(stringId));
    }
}

void OptimizedCommandDispatcher::unregisterListener(CommandId commandId, std::shared_ptr<CommandListener> listener) {
    if (!listener) {
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_listeners.find(commandId);
    if (it != m_listeners.end()) {
        auto& listeners = it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
        
        if (listeners.empty()) {
            m_listeners.erase(it);
        }
        
        LOG_INF_S("Unregistered listener '" + listener->getListenerName() + "' for command ID: " + std::to_string(commandId));
    }
}

OptimizedCommandDispatcher::CommandResult OptimizedCommandDispatcher::dispatchCommand(CommandId commandId, const CommandParameters& parameters) {
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_listeners.find(commandId);
    if (it == m_listeners.end() || it->second.empty()) {
        std::string errorMsg = "No listeners registered for command ID: " + std::to_string(commandId);
        LOG_ERR_S(errorMsg);
        CommandResult result(false, errorMsg, commandId);
        
        if (m_uiFeedbackHandler) {
            m_uiFeedbackHandler(result);
        }
        
        return result;
    }
    
    // Execute command with the first available listener
    auto& listeners = it->second;
    for (auto& listener : listeners) {
        // Get the original command string from our reverse mapping
        std::string commandString;
        auto stringIt = m_commandIdToString.find(commandId);
        if (stringIt != m_commandIdToString.end()) {
            commandString = stringIt->second;
        } else {
            // Fallback: try to convert as enum ID
            try {
                cmd::CommandType commandType = static_cast<cmd::CommandType>(commandId);
                commandString = cmd::to_string(commandType);
            } catch (...) {
                commandString = "UNKNOWN";
            }
        }
        
        if (listener && listener->canHandleCommand(commandString)) {
            try {
                auto externalResult = listener->executeCommand(commandString, parameters);
                OptimizedCommandDispatcher::CommandResult result(externalResult.success, externalResult.message, commandId);
                
                
                if (m_uiFeedbackHandler) {
                    m_uiFeedbackHandler(result);
                }
                
                return result;
            }
            catch (const std::exception& e) {
                std::string errorMsg = "Exception in command execution: " + std::string(e.what());
                LOG_ERR_S(errorMsg);
                OptimizedCommandDispatcher::CommandResult result(false, errorMsg, commandId);
                
                if (m_uiFeedbackHandler) {
                    m_uiFeedbackHandler(result);
                }
                
                return result;
            }
        }
    }
    
    std::string errorMsg = "No capable listener found for command ID: " + std::to_string(commandId);
    LOG_ERR_S(errorMsg);
    OptimizedCommandDispatcher::CommandResult result(false, errorMsg, commandId);
    
    if (m_uiFeedbackHandler) {
        m_uiFeedbackHandler(result);
    }
    
    return result;
}

bool OptimizedCommandDispatcher::hasHandler(CommandId commandId) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_listeners.find(commandId);
    return it != m_listeners.end() && !it->second.empty();
}

void OptimizedCommandDispatcher::setUIFeedbackHandler(std::function<void(const CommandResult&)> handler) {
    m_uiFeedbackHandler = handler;
    LOG_INF_S("UI feedback handler registered for optimized command dispatcher");
}

OptimizedCommandDispatcher::CommandId OptimizedCommandDispatcher::stringToCommandId(const std::string& commandString) {
    // Simple hash function for string to ID conversion
    std::hash<std::string> hasher;
    return static_cast<CommandId>(hasher(commandString));
}

OptimizedCommandDispatcher::CommandId OptimizedCommandDispatcher::commandTypeToId(cmd::CommandType commandType) {
    return static_cast<CommandId>(commandType);
}

// ============================================================================
// GeometryComputationCache Implementation
// ============================================================================

GeometryComputationCache::GeometryKey::GeometryKey(const std::string& t, const std::vector<double>& p) 
    : type(t), hash(0) {
    std::fill(std::begin(params), std::end(params), 0.0);
    for (size_t i = 0; i < std::min(p.size(), size_t(6)); ++i) {
        params[i] = p[i];
    }
    
    // Compute hash
    std::hash<std::string> stringHasher;
    std::hash<double> doubleHasher;
    hash = stringHasher(type);
    for (double param : params) {
        hash ^= doubleHasher(param) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
}

bool GeometryComputationCache::GeometryKey::operator==(const GeometryKey& other) const {
    if (type != other.type) return false;
    for (int i = 0; i < 6; ++i) {
        if (std::abs(params[i] - other.params[i]) > 1e-9) return false;
    }
    return true;
}

GeometryComputationCache::GeometryComputationCache() 
    : m_threadPool(std::make_unique<ThreadPool>()) {
    LOG_INF_S("GeometryComputationCache initialized");
}

GeometryComputationCache::~GeometryComputationCache() {
    if (m_threadPool) {
        m_threadPool->stop();
        m_threadPool.reset();
    }
    LOG_INF_S("GeometryComputationCache destroyed");
}

TopoDS_Shape GeometryComputationCache::getOrCreateGeometry(const GeometryKey& key, 
                                                          std::function<TopoDS_Shape()> creator) {
    size_t hash = key.hash;
    
    {
        std::shared_lock<std::shared_mutex> lock(m_geometryMutex);
        auto it = m_geometryCache.find(hash);
        if (it != m_geometryCache.end()) {
            it->second.accessCount++;
            it->second.timestamp = std::chrono::steady_clock::now();
            return it->second.shape;
        }
    }
    
    // Create new geometry
    auto shape = creator();
    
    {
        std::unique_lock<std::shared_mutex> lock(m_geometryMutex);
        // Check again in case another thread created it
        auto it = m_geometryCache.find(hash);
        if (it != m_geometryCache.end()) {
            it->second.accessCount++;
            return it->second.shape;
        }
        
        // Add to cache
        if (m_geometryCache.size() >= MAX_CACHE_SIZE) {
            cleanupExpiredEntries();
        }
        
        m_geometryCache[hash] = CachedGeometry(shape);
    }
    
    return shape;
}

OCCMeshConverter::TriangleMesh GeometryComputationCache::getOrCreateMesh(const TopoDS_Shape& shape, 
                                                                        const OCCMeshConverter::MeshParameters& params,
                                                                        std::function<OCCMeshConverter::TriangleMesh()> creator) {
    size_t hash = computeMeshHash(shape, params);
    
    {
        std::shared_lock<std::shared_mutex> lock(m_meshMutex);
        auto it = m_meshCache.find(hash);
        if (it != m_meshCache.end()) {
            it->second.accessCount++;
            it->second.timestamp = std::chrono::steady_clock::now();
            return it->second.mesh;
        }
    }
    
    // Create new mesh
    auto mesh = creator();
    
    {
        std::unique_lock<std::shared_mutex> lock(m_meshMutex);
        // Check again in case another thread created it
        auto it = m_meshCache.find(hash);
        if (it != m_meshCache.end()) {
            it->second.accessCount++;
            return it->second.mesh;
        }
        
        // Add to cache
        if (m_meshCache.size() >= MAX_CACHE_SIZE) {
            cleanupExpiredEntries();
        }
        
        m_meshCache[hash] = MeshCacheEntry(mesh);
    }
    
    return mesh;
}

std::future<TopoDS_Shape> GeometryComputationCache::createGeometryAsync(const GeometryKey& key,
                                                                       std::function<TopoDS_Shape()> creator) {
    return m_threadPool->enqueue([this, key, creator]() {
        return getOrCreateGeometry(key, creator);
    });
}

std::future<OCCMeshConverter::TriangleMesh> GeometryComputationCache::createMeshAsync(const TopoDS_Shape& shape,
                                                                                     const OCCMeshConverter::MeshParameters& params,
                                                                                     std::function<OCCMeshConverter::TriangleMesh()> creator) {
    return m_threadPool->enqueue([this, shape, params, creator]() {
        return getOrCreateMesh(shape, params, creator);
    });
}

void GeometryComputationCache::cleanupExpiredEntries() {
    auto now = std::chrono::steady_clock::now();
    
    // Clean geometry cache
    for (auto it = m_geometryCache.begin(); it != m_geometryCache.end();) {
        if (now - it->second.timestamp > CACHE_TTL) {
            it = m_geometryCache.erase(it);
        } else {
            ++it;
        }
    }
    
    // Clean mesh cache
    for (auto it = m_meshCache.begin(); it != m_meshCache.end();) {
        if (now - it->second.timestamp > CACHE_TTL) {
            it = m_meshCache.erase(it);
        } else {
            ++it;
        }
    }
}

size_t GeometryComputationCache::computeGeometryHash(const GeometryKey& key) {
    return key.hash;
}

size_t GeometryComputationCache::computeMeshHash(const TopoDS_Shape& shape, const OCCMeshConverter::MeshParameters& params) {
    std::hash<double> doubleHasher;
    std::hash<bool> boolHasher;
    
    size_t hash = 0;
    hash ^= doubleHasher(params.deflection) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= doubleHasher(params.angularDeflection) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= boolHasher(params.relative) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= boolHasher(params.inParallel) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    
    // Add shape hash
    hash ^= std::hash<TopoDS_Shape>{}(shape) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    
    return hash;
}

void GeometryComputationCache::clearCache() {
    std::unique_lock<std::shared_mutex> geoLock(m_geometryMutex);
    std::unique_lock<std::shared_mutex> meshLock(m_meshMutex);
    m_geometryCache.clear();
    m_meshCache.clear();
    LOG_INF_S("Geometry computation cache cleared");
}

void GeometryComputationCache::cleanup() {
    cleanupExpiredEntries();
}

size_t GeometryComputationCache::getCacheSize() const {
    std::shared_lock<std::shared_mutex> lock(m_geometryMutex);
    return m_geometryCache.size();
}

size_t GeometryComputationCache::getMeshCacheSize() const {
    std::shared_lock<std::shared_mutex> lock(m_meshMutex);
    return m_meshCache.size();
}

// ============================================================================
// OptimizedGeometryManager Implementation
// ============================================================================

OptimizedGeometryManager::OptimizedGeometryManager() {
    LOG_INF_S("OptimizedGeometryManager initialized");
}

OptimizedGeometryManager::~OptimizedGeometryManager() {
    LOG_INF_S("OptimizedGeometryManager destroyed");
}

void OptimizedGeometryManager::addGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) {
        LOG_ERR_S("Attempted to add null geometry");
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    const std::string& name = geometry->getName();
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        LOG_WRN_S("Geometry with name '" + name + "' already exists");
        return;
    }
    
    m_geometryMap[name] = geometry;
    m_geometryList.push_back(geometry);
    
    LOG_INF_S("Added geometry: " + name);
}

void OptimizedGeometryManager::removeGeometry(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        auto geometry = it->second;
        m_geometryMap.erase(it);
        
        // Remove from list
        auto listIt = std::find(m_geometryList.begin(), m_geometryList.end(), geometry);
        if (listIt != m_geometryList.end()) {
            m_geometryList.erase(listIt);
        }
        
        // Remove from selected
        m_selectedGeometries.erase(name);
        
        LOG_INF_S("Removed geometry: " + name);
    }
}

void OptimizedGeometryManager::removeGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (geometry) {
        removeGeometry(geometry->getName());
    }
}

std::shared_ptr<OCCGeometry> OptimizedGeometryManager::findGeometry(const std::string& name) {
    m_metrics.lookupCount++;
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        m_metrics.cacheHits++;
        return it->second;
    }
    
    m_metrics.cacheMisses++;
    return nullptr;
}

std::vector<std::shared_ptr<OCCGeometry>> OptimizedGeometryManager::getAllGeometries() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_geometryList;
}

std::vector<std::shared_ptr<OCCGeometry>> OptimizedGeometryManager::getSelectedGeometries() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    std::vector<std::shared_ptr<OCCGeometry>> selected;
    selected.reserve(m_selectedGeometries.size());
    
    for (const auto& [name, geometry] : m_selectedGeometries) {
        selected.push_back(geometry);
    }
    
    return selected;
}

void OptimizedGeometryManager::selectGeometry(const std::string& name, bool selected) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        if (selected) {
            m_selectedGeometries[name] = it->second;
        } else {
            m_selectedGeometries.erase(name);
        }
    }
}

void OptimizedGeometryManager::selectAll() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_selectedGeometries = m_geometryMap;
}

void OptimizedGeometryManager::deselectAll() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_selectedGeometries.clear();
}

void OptimizedGeometryManager::addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    for (const auto& geometry : geometries) {
        if (geometry) {
            const std::string& name = geometry->getName();
            if (m_geometryMap.find(name) == m_geometryMap.end()) {
                m_geometryMap[name] = geometry;
                m_geometryList.push_back(geometry);
            }
        }
    }
}

void OptimizedGeometryManager::removeGeometries(const std::vector<std::string>& names) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    for (const auto& name : names) {
        auto it = m_geometryMap.find(name);
        if (it != m_geometryMap.end()) {
            auto geometry = it->second;
            m_geometryMap.erase(it);
            
            // Remove from list
            auto listIt = std::find(m_geometryList.begin(), m_geometryList.end(), geometry);
            if (listIt != m_geometryList.end()) {
                m_geometryList.erase(listIt);
            }
            
            // Remove from selected
            m_selectedGeometries.erase(name);
        }
    }
}

void OptimizedGeometryManager::resetMetrics() {
    m_metrics.lookupCount = 0;
    m_metrics.cacheHits = 0;
    m_metrics.cacheMisses = 0;
}

// ============================================================================
// PerformanceOptimizer Implementation
// ============================================================================

PerformanceOptimizer::PerformanceOptimizer() {
    m_metrics.startTime = std::chrono::steady_clock::now();
    LOG_INF_S("PerformanceOptimizer created");
}

PerformanceOptimizer::~PerformanceOptimizer() {
    cleanup();
    LOG_INF_S("PerformanceOptimizer destroyed");
}

void PerformanceOptimizer::initialize(const Config& config) {
    m_config = config;
    
    if (m_config.enableCommandOptimization) {
        m_commandDispatcher = std::make_unique<OptimizedCommandDispatcher>();
    }
    
    if (m_config.enableGeometryCaching) {
        m_geometryCache = std::make_unique<GeometryComputationCache>();
    }
    
    if (m_config.enableContainerOptimization) {
        m_geometryManager = std::make_unique<OptimizedGeometryManager>();
    }
    
    LOG_INF_S("PerformanceOptimizer initialized with optimizations enabled");
}

void PerformanceOptimizer::startTiming(const std::string& operation) {
    (void)operation; // Suppress unused parameter warning
    // Timing is handled by the PERFORMANCE_TIMING macro
}

void PerformanceOptimizer::endTiming(const std::string& operation) {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = endTime - m_metrics.startTime;
    
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_metrics.mutex));
    m_metrics.timings[operation] = duration;
}

void PerformanceOptimizer::printPerformanceReport() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_metrics.mutex));
    m_metrics.printReport();
}

void PerformanceOptimizer::setConfig(const Config& config) {
    m_config = config;
    LOG_INF_S("PerformanceOptimizer configuration updated");
}

void PerformanceOptimizer::cleanup() {
    if (m_geometryCache) {
        m_geometryCache->cleanup();
        m_geometryCache.reset();
    }
    
    if (m_commandDispatcher) {
        m_commandDispatcher.reset();
    }
    
    if (m_geometryManager) {
        m_geometryManager.reset();
    }
    
    LOG_INF_S("PerformanceOptimizer cleanup completed");
}

// ============================================================================
// PerformanceMetrics Implementation
// ============================================================================

void PerformanceOptimizer::PerformanceMetrics::recordTiming(const std::string& operation, std::chrono::nanoseconds duration) {
    std::lock_guard<std::mutex> lock(mutex);
    timings[operation] = duration;
}

void PerformanceOptimizer::PerformanceMetrics::printReport() const {
    std::cout << "\n=== Performance Report ===" << std::endl;
    for (const auto& [operation, duration] : timings) {
        std::cout << operation << ": " << duration.count() / 1000000.0 << "ms" << std::endl;
    }
    std::cout << "=========================" << std::endl;
}

// ============================================================================
// ThreadPool Implementation
// ============================================================================

GeometryComputationCache::ThreadPool::ThreadPool(size_t threadCount) {
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_queueMutex);
                    m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                    if (m_stop && m_tasks.empty()) {
                        return;
                    }
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }
                task();
            }
        });
    }
}

GeometryComputationCache::ThreadPool::~ThreadPool() {
    stop();
}

void GeometryComputationCache::ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
        // Clear any pending tasks
        while (!m_tasks.empty()) {
            m_tasks.pop();
        }
    }
    m_condition.notify_all();
    
    // Wait for all workers to finish
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    m_workers.clear();
}

// ============================================================================
// ThreadPool Implementation
// ============================================================================

template<class F, class... Args>
auto GeometryComputationCache::ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_stop) {
            // Return a future with an exception instead of throwing
            std::promise<return_type> promise;
            promise.set_exception(std::make_exception_ptr(std::runtime_error("ThreadPool is stopped")));
            return promise.get_future();
        }
        m_tasks.emplace([task](){ (*task)(); });
    }
    m_condition.notify_one();
    return res;
}

} // namespace optimizer