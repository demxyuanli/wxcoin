#include "async/AsyncComputeEngine.h"
#include "logger/Logger.h"
#include <algorithm>
#include <tbb/tbb.h>
#include <tbb/global_control.h>

namespace async {

AsyncComputeEngine::AsyncComputeEngine(const Config& config)
    : m_config(config)
    , m_running(true)
    , m_paused(false)
    , m_shutdown(false)
{
    LOG_INF_S("AsyncComputeEngine: Initializing with TBB task scheduler");

    if (m_config.numWorkerThreads > 0) {
        static tbb::global_control global_limit(
            tbb::global_control::max_allowed_parallelism, 
            m_config.numWorkerThreads
        );
    }

    LOG_INF_S("AsyncComputeEngine: Initialized successfully with TBB");
}

AsyncComputeEngine::~AsyncComputeEngine() {
    shutdown();
}

void AsyncComputeEngine::shutdown() {
    if (m_shutdown.load()) {
        return;
    }

    LOG_INF_S("AsyncComputeEngine: Shutting down TBB tasks...");

    m_shutdown.store(true);
    m_running.store(false);

    // Cancel all pending tasks and wait for completion
    m_taskGroup.cancel();

    try {
        m_taskGroup.wait();
    } catch (const std::exception& e) {
        LOG_WRN_S("AsyncComputeEngine: Exception during shutdown: " + std::string(e.what()));
    }

    // Clear all data structures
    m_taskQueue.clear();
    m_activeTasks.clear();
    m_sharedDataCache.clear();

    LOG_INF_S("AsyncComputeEngine: TBB shutdown complete");
}


void AsyncComputeEngine::cancelTask(const std::string& taskId) {
    auto it = m_activeTasks.find(taskId);
    if (it != m_activeTasks.end()) {
        it->second();

        std::lock_guard<std::mutex> statsLock(m_statisticsMutex);
        if (m_statistics.runningTasks > 0) {
            m_statistics.runningTasks--;
        }
        m_statistics.failedTasks++;

        LOG_INF_S("AsyncComputeEngine: Cancelled task " + taskId);
    }
}

void AsyncComputeEngine::cancelAllTasks() {
    m_taskGroup.cancel();

    try {
        m_taskGroup.wait();
    } catch (const std::exception& e) {
        LOG_WRN_S("AsyncComputeEngine: Exception during cancel: " + std::string(e.what()));
    }

    for (auto& [taskId, cancelFunc] : m_activeTasks) {
        try {
            cancelFunc();
        } catch (const std::exception& e) {
            LOG_WRN_S("AsyncComputeEngine: Exception cancelling task " + taskId + ": " + std::string(e.what()));
        }
    }

    std::lock_guard<std::mutex> statsLock(m_statisticsMutex);
    m_statistics.runningTasks = 0;

    LOG_INF_S("AsyncComputeEngine: Cancelled all tasks");
}

void AsyncComputeEngine::pause() {
    m_paused.store(true);
    LOG_INF_S("AsyncComputeEngine: Tasks paused");
}

void AsyncComputeEngine::resume() {
    m_paused.store(false);
    LOG_INF_S("AsyncComputeEngine: Tasks resumed");
}

TaskStatistics AsyncComputeEngine::getStatistics() const {
    std::lock_guard<std::mutex> lock(m_statisticsMutex);
    return m_statistics;
}

size_t AsyncComputeEngine::getQueueSize() const {
    return m_taskQueue.size();
}

size_t AsyncComputeEngine::getActiveTaskCount() const {
    return m_activeTasks.size();
}

void AsyncComputeEngine::updateTaskStatistics(bool success) {
    std::lock_guard<std::mutex> statsLock(m_statisticsMutex);
    if (success) {
        m_statistics.completedTasks++;
    } else {
        m_statistics.failedTasks++;
    }
    if (m_statistics.runningTasks > 0) {
        m_statistics.runningTasks--;
    }
    m_statistics.totalProcessedTasks++;
}

void AsyncComputeEngine::cleanupExpiredCache() {
    std::vector<std::string> expiredKeys;
    std::vector<std::pair<std::string, double>> lruScores;

    for (const auto& [key, entry] : m_sharedDataCache) {
        if (entry->getAgeMinutes() > m_config.cacheExpirationTime.count()) {
            expiredKeys.push_back(key);
        } else {
            lruScores.emplace_back(key, entry->getLRUScore());
        }
    }

    if (!expiredKeys.empty()) {
        LOG_DBG_S("AsyncComputeEngine: Found " + std::to_string(expiredKeys.size()) + " expired cache entries");
    }

    if (m_sharedDataCache.size() > m_config.maxCacheSize && !lruScores.empty()) {
        size_t targetSize = m_config.maxCacheSize * 4 / 5;
        size_t itemsToRemove = m_sharedDataCache.size() - targetSize;
        
        std::sort(lruScores.begin(), lruScores.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        size_t actualRemove = std::min(itemsToRemove, lruScores.size());
        LOG_DBG_S("AsyncComputeEngine: Need to remove " + std::to_string(actualRemove) + " items via LRU");
    }

    LOG_DBG_S("AsyncComputeEngine: Cache size: " + std::to_string(m_sharedDataCache.size()));
}

void AsyncComputeEngine::removeSharedData(const std::string& key) {
    LOG_DBG_S("AsyncComputeEngine: Marking cache entry '" + key + "' for removal");
}

size_t AsyncComputeEngine::getCacheSize() const {
    return m_sharedDataCache.size();
}

size_t AsyncComputeEngine::getCacheMemoryUsage() const {
    size_t total = 0;
    for (const auto& [key, entry] : m_sharedDataCache) {
        total += entry->memoryUsage;
    }
    return total;
}

} // namespace async

