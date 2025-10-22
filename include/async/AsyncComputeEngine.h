#pragma once

// Windows header order protection
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <string>
#include <vector>
#include <tbb/tbb.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_priority_queue.h>
#include <tbb/task_group.h>
#include "logger/Logger.h"

namespace async {

// Forward declarations
struct MeshData;

enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

enum class TaskState {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

struct TaskStatistics {
    size_t queuedTasks{0};
    size_t runningTasks{0};
    size_t completedTasks{0};
    size_t failedTasks{0};
    double avgExecutionTimeMs{0.0};
    size_t totalProcessedTasks{0};
};

template<typename ResultType>
struct ComputeResult {
    bool success{false};
    ResultType data;
    std::string errorMessage;
    std::chrono::milliseconds executionTime{0};
    
    ComputeResult() = default;
    ComputeResult(const ResultType& result) : success(true), data(result) {}
    ComputeResult(const std::string& error) : success(false), errorMessage(error) {}
};

// Generic async task template for universal task support
template<typename InputType, typename OutputType>
class GenericAsyncTask {
public:
    using Input = InputType;
    using Output = OutputType;
    using TaskFunction = std::function<Output(const Input&, std::atomic<bool>&, std::function<void(int, const std::string&)>&)>;
    using ProgressCallback = std::function<void(int, const std::string&)>;

    GenericAsyncTask(const std::string& id, const Input& input, TaskFunction func,
                    ProgressCallback progressCb = nullptr)
        : m_taskId(id), m_input(input), m_function(func), m_progressCallback(progressCb), m_cancelled(false) {}

    const std::string& getTaskId() const { return m_taskId; }
    const Input& getInput() const { return m_input; }

    Output execute() {
        return m_function(m_input, m_cancelled, m_progressCallback);
    }

    void cancel() {
        m_cancelled.store(true);
    }

    bool isCancelled() const {
        return m_cancelled.load();
    }

private:
    std::string m_taskId;
    Input m_input;
    TaskFunction m_function;
    ProgressCallback m_progressCallback;
    std::atomic<bool> m_cancelled;
};

template<typename InputType, typename ResultType>
class AsyncTask {
public:
    using ProgressFunc = std::function<void(int, const std::string&)>;
    using ComputeFunc = std::function<ResultType(const InputType&, std::atomic<bool>&, ProgressFunc)>;
    using PartialResultFunc = std::function<void(const ResultType&)>;
    using CompletionFunc = std::function<void(const ComputeResult<ResultType>&)>;
    
    struct Config {
        TaskPriority priority{TaskPriority::Normal};
        bool cacheResult{true};
        bool supportCancellation{true};
        bool enableProgressCallback{false};
        bool enablePartialResults{false};
        size_t partialResultBatchSize{50};
    };
    
    AsyncTask(
        const std::string& taskId,
        const InputType& input,
        ComputeFunc computeFunc,
        CompletionFunc completionFunc,
        const Config& config = Config())
        : m_taskId(taskId)
        , m_input(input)
        , m_computeFunc(computeFunc)
        , m_completionFunc(completionFunc)
        , m_config(config)
        , m_state(TaskState::Pending)
        , m_cancelled(false)
    {}
    
    void setProgressCallback(ProgressFunc callback) { m_progressFunc = callback; }
    void setPartialResultCallback(PartialResultFunc callback) { m_partialResultFunc = callback; }
    
    void execute() {
        if (m_state != TaskState::Pending) return;
        
        m_state = TaskState::Running;
        auto startTime = std::chrono::steady_clock::now();
        
        try {
            ResultType result = m_computeFunc(m_input, m_cancelled, m_progressFunc);

            if (m_cancelled.load()) {
                m_state = TaskState::Cancelled;
                return;
            }
            
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            ComputeResult<ResultType> computeResult(result);
            computeResult.executionTime = duration;
            
            if (m_completionFunc) {
                m_completionFunc(computeResult);
            }
            
            m_state = TaskState::Completed;
        }
        catch (const std::exception& e) {
            ComputeResult<ResultType> errorResult(e.what());
            if (m_completionFunc) {
                m_completionFunc(errorResult);
            }
            m_state = TaskState::Failed;
        }
    }
    
    void cancel() {
        if (m_config.supportCancellation) {
            m_cancelled.store(true);
        }
    }
    
    bool isCancelled() const { return m_cancelled.load(); }
    TaskState getState() const { return m_state; }
    const std::string& getTaskId() const { return m_taskId; }
    TaskPriority getPriority() const { return m_config.priority; }
    
    void updateProgress(int progress, const std::string& message) {
        if (m_config.enableProgressCallback && m_progressFunc) {
            m_progressFunc(progress, message);
        }
    }
    
    void reportPartialResult(const ResultType& partial) {
        if (m_config.enablePartialResults && m_partialResultFunc) {
            m_partialResultFunc(partial);
        }
    }

private:
    std::string m_taskId;
    InputType m_input;
    ComputeFunc m_computeFunc;
    CompletionFunc m_completionFunc;
    ProgressFunc m_progressFunc;
    PartialResultFunc m_partialResultFunc;
    Config m_config;
    
    std::atomic<TaskState> m_state;
    std::atomic<bool> m_cancelled;
};

// Cache entry base class - supports LRU eviction
struct CacheEntry {
    std::chrono::steady_clock::time_point lastAccessTime;
    std::chrono::steady_clock::time_point createTime;
    size_t accessCount{0};
    size_t memoryUsage{0};

    CacheEntry() :
        lastAccessTime(std::chrono::steady_clock::now()),
        createTime(std::chrono::steady_clock::now()) {}

    virtual ~CacheEntry() = default;

    void updateAccess() {
        lastAccessTime = std::chrono::steady_clock::now();
        accessCount++;
    }

    size_t getAgeMinutes() const {
        return std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::steady_clock::now() - createTime).count();
    }

    size_t getLastAccessMinutes() const {
        return std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::steady_clock::now() - lastAccessTime).count();
    }

    double getLRUScore() const {
        double recency = 1.0 / (1.0 + getLastAccessMinutes());
        double frequency = std::min(1.0, accessCount / 10.0);
        return recency * 0.7 + frequency * 0.3;
    }
};

template<typename T>
struct SharedComputeData : public CacheEntry {
    std::shared_ptr<T> data;
    std::atomic<bool> ready{false};
    std::atomic<size_t> refCount{0};

    SharedComputeData() = default;
    explicit SharedComputeData(std::shared_ptr<T> d)
        : data(d)
        , ready(true)
    {
        memoryUsage = sizeof(T);
    }
    
    void setMemoryUsage(size_t size) {
        memoryUsage = size;
    }
};

class AsyncComputeEngine {
public:
    struct Config {
        size_t numWorkerThreads{0};
        size_t maxQueueSize{1000};
        bool enableResultCache{true};
        size_t maxCacheSize{100};
        std::chrono::minutes cacheExpirationTime{30};
    };
    
    explicit AsyncComputeEngine(const Config& config = Config());
    ~AsyncComputeEngine();
    
    AsyncComputeEngine(const AsyncComputeEngine&) = delete;
    AsyncComputeEngine& operator=(const AsyncComputeEngine&) = delete;
    
    template<typename InputType, typename ResultType>
    void submitTask(std::shared_ptr<AsyncTask<InputType, ResultType>> task);

    template<typename InputType, typename OutputType>
    void submitGenericTask(std::shared_ptr<GenericAsyncTask<InputType, OutputType>> task,
                          std::function<void(const OutputType&)> onComplete = nullptr);
    
    void cancelTask(const std::string& taskId);
    void cancelAllTasks();
    
    // Global progress callback
    void setGlobalProgressCallback(std::function<void(const std::string&, int, const std::string&)> callback) {
        m_globalProgressCallback = callback;
    }
    
    void pause();
    void resume();
    void shutdown();
    
    TaskStatistics getStatistics() const;
    
    template<typename T>
    std::shared_ptr<SharedComputeData<T>> getSharedData(const std::string& key);

    template<typename T>
    void setSharedData(const std::string& key, std::shared_ptr<T> data);

    void removeSharedData(const std::string& key);

    size_t getCacheSize() const;
    size_t getCacheMemoryUsage() const;
    
    size_t getQueueSize() const;
    size_t getActiveTaskCount() const;
    bool isRunning() const { return m_running.load(); }

private:
    struct TaskWrapper {
        std::function<void()> execute;
        TaskPriority priority;
        std::chrono::steady_clock::time_point submitTime;
        std::string taskId;

        bool operator<(const TaskWrapper& other) const {
            if (priority != other.priority) {
                return static_cast<int>(priority) < static_cast<int>(other.priority);
            }
            return submitTime > other.submitTime;
        }
    };

    void cleanupExpiredCache();
    void updateTaskStatistics(bool success);

    Config m_config;

    tbb::task_group m_taskGroup;

    tbb::concurrent_priority_queue<TaskWrapper> m_taskQueue;
    tbb::concurrent_unordered_map<std::string, std::function<void()>> m_activeTasks;
    tbb::concurrent_unordered_map<std::string, std::unique_ptr<CacheEntry>> m_sharedDataCache;

    std::atomic<bool> m_running{true};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_shutdown{false};

    TaskStatistics m_statistics;
    mutable std::mutex m_statisticsMutex;
    
    // Global progress callback
    std::function<void(const std::string&, int, const std::string&)> m_globalProgressCallback;
};

// Template implementations must be in header for linking
template<typename InputType, typename ResultType>
void AsyncComputeEngine::submitTask(std::shared_ptr<AsyncTask<InputType, ResultType>> task) {
    if (!m_running.load() || m_shutdown.load()) {
        return;
    }

    if (m_taskQueue.size() >= m_config.maxQueueSize) {
        throw std::runtime_error("Task queue is full");
    }

    std::string taskId = task->getTaskId();

    m_activeTasks[taskId] = [task]() { task->cancel(); };

    {
        std::lock_guard<std::mutex> statsLock(m_statisticsMutex);
        m_statistics.queuedTasks++;
        m_statistics.runningTasks++;
    }

    m_taskGroup.run([this, task]() {
        if (!m_paused.load() && m_running.load()) {
            try {
                task->execute();
                updateTaskStatistics(true);
            } catch (const std::exception& e) {
                LOG_ERR_S("Task execution failed: " + std::string(e.what()));
                updateTaskStatistics(false);
            }
        }
    });
    
    LOG_DBG_S("AsyncComputeEngine: Task '" + task->getTaskId() + "' submitted");
}

template<typename InputType, typename OutputType>
void AsyncComputeEngine::submitGenericTask(std::shared_ptr<GenericAsyncTask<InputType, OutputType>> task,
                                          std::function<void(const OutputType&)> onComplete) {
    if (!m_running.load() || m_shutdown.load()) {
        return;
    }

    if (m_taskQueue.size() >= m_config.maxQueueSize) {
        throw std::runtime_error("Task queue is full");
    }

    m_activeTasks[task->getTaskId()] = [task]() { task->cancel(); };

    {
        std::lock_guard<std::mutex> statsLock(m_statisticsMutex);
        m_statistics.queuedTasks++;
        m_statistics.runningTasks++;
    }

    m_taskGroup.run([this, task, onComplete]() {
        if (!m_paused.load() && m_running.load()) {
            try {
                OutputType result = task->execute();

                if (onComplete) {
                    onComplete(result);
                }

                updateTaskStatistics(true);
            } catch (const std::exception& e) {
                LOG_ERR_S("Generic task execution failed: " + std::string(e.what()));
                updateTaskStatistics(false);
            }
        }
    });

    LOG_DBG_S("AsyncComputeEngine: Generic task '" + task->getTaskId() + "' submitted");
}

template<typename T>
std::shared_ptr<SharedComputeData<T>> AsyncComputeEngine::getSharedData(const std::string& key) {
    auto it = m_sharedDataCache.find(key);
    if (it != m_sharedDataCache.end()) {
        auto* sharedData = dynamic_cast<SharedComputeData<T>*>(it->second.get());
        if (sharedData) {
            sharedData->updateAccess();
            sharedData->refCount++;
            return std::shared_ptr<SharedComputeData<T>>(
                sharedData, 
                [](SharedComputeData<T>*) { /* Non-owning shared_ptr */ }
            );
        }
    }
    return nullptr;
}

template<typename T>
void AsyncComputeEngine::setSharedData(const std::string& key, std::shared_ptr<T> data) {
    auto sharedData = std::make_unique<SharedComputeData<T>>(data);
    m_sharedDataCache[key] = std::move(sharedData);

    if (m_sharedDataCache.size() > m_config.maxCacheSize) {
        cleanupExpiredCache();
    }
}

} // namespace async
