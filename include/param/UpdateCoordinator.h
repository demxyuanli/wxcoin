#pragma once

#include "ParameterRegistry.h"
#include "UnifiedParameterTree.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

// Forward declarations
class SceneManager;
class RenderingEngine;
class OCCViewer;

/**
 * @brief Update task types
 */
enum class UpdateTaskType {
    PARAMETER_CHANGE,      // Parameter change
    GEOMETRY_REBUILD,      // Geometry rebuild
    RENDERING_UPDATE,      // Rendering update
    LIGHTING_UPDATE,       // Lighting update
    DISPLAY_UPDATE,        // Display update
    PERFORMANCE_UPDATE,    // Performance update
    BATCH_UPDATE          // Batch update
};

/**
 * @brief Update task
 */
struct UpdateTask {
    UpdateTaskType type;
    std::string targetPath;                    // Target parameter path
    ParameterValue oldValue;                   // Old value
    ParameterValue newValue;                    // New value
    std::chrono::steady_clock::time_point timestamp;
    int priority;                              // Priority (0-10, 10 highest)
    std::unordered_set<std::string> dependencies; // Dependent task IDs
    std::string taskId;                        // Unique task ID
    bool isBatchable;                          // Whether batchable
    std::function<void()> executeFunction;     // Execution function
};

/**
 * @brief Batch update group
 */
struct BatchUpdateGroup {
    std::string groupId;
    std::vector<UpdateTask> tasks;
    std::chrono::steady_clock::time_point createdTime;
    std::chrono::milliseconds maxWaitTime;
    bool isExecuting;
};

/**
 * @brief Update strategies
 */
enum class UpdateStrategy {
    IMMEDIATE,          // Immediate execution
    BATCHED,            // Batch execution
    THROTTLED,          // Throttled execution
    DEFERRED            // Deferred execution
};

/**
 * @brief Update coordinator
 * Intelligently coordinates parameter changes and rendering updates, provides batch processing and dependency management
 */
class UpdateCoordinator {
public:
    // Update callback function types
    using UpdateCallback = std::function<void(const UpdateTask&)>;
    using BatchUpdateCallback = std::function<void(const BatchUpdateGroup&)>;
    using CompletionCallback = std::function<void(const std::string& taskId, bool success)>;

    static UpdateCoordinator& getInstance();

    // Initialization
    bool initialize();
    void shutdown();

    // Task submission
    std::string submitUpdateTask(const UpdateTask& task);
    std::string submitParameterChange(
        const std::string& parameterPath,
        const ParameterValue& oldValue,
        const ParameterValue& newValue,
        UpdateStrategy strategy = UpdateStrategy::BATCHED
    );

    // Batch task submission
    std::string submitBatchUpdate(
        const std::vector<UpdateTask>& tasks,
        const std::string& groupId = ""
    );

    // Task management
    bool cancelTask(const std::string& taskId);
    bool isTaskPending(const std::string& taskId) const;
    bool isTaskExecuting(const std::string& taskId) const;
    std::vector<std::string> getPendingTasks() const;
    std::vector<std::string> getExecutingTasks() const;

    // Batch processing control
    void setBatchProcessingEnabled(bool enabled) { m_batchProcessingEnabled = enabled; }
    bool isBatchProcessingEnabled() const { return m_batchProcessingEnabled; }
    void setBatchTimeout(std::chrono::milliseconds timeout) { m_batchTimeout = timeout; }
    std::chrono::milliseconds getBatchTimeout() const { return m_batchTimeout; }

    // Dependency management
    void addTaskDependency(const std::string& taskId, const std::string& dependencyTaskId);
    void removeTaskDependency(const std::string& taskId, const std::string& dependencyTaskId);
    std::vector<std::string> getTaskDependencies(const std::string& taskId) const;

    // Priority management
    void setTaskPriority(const std::string& taskId, int priority);
    int getTaskPriority(const std::string& taskId) const;

    // Callback registration
    int registerUpdateCallback(UpdateCallback callback);
    int registerBatchUpdateCallback(BatchUpdateCallback callback);
    int registerCompletionCallback(CompletionCallback callback);
    void unregisterCallback(int callbackId);

    // Execution control
    void pauseExecution();
    void resumeExecution();
    bool isExecutionPaused() const { return m_executionPaused; }

    // Performance monitoring
    struct PerformanceMetrics {
        size_t totalTasksSubmitted;
        size_t totalTasksExecuted;
        size_t totalBatchGroups;
        size_t averageBatchSize;
        std::chrono::milliseconds averageExecutionTime;
        std::chrono::milliseconds averageWaitTime;
        size_t dependencyConflicts;
        size_t cancelledTasks;
    };
    PerformanceMetrics getPerformanceMetrics() const;
    void resetPerformanceMetrics();

    // Smart batch processing
    void enableSmartBatching(bool enabled) { m_smartBatchingEnabled = enabled; }
    bool isSmartBatchingEnabled() const { return m_smartBatchingEnabled; }
    
    // Batch grouping strategy
    enum class BatchGroupingStrategy {
        BY_TYPE,            // 按任务类型分组
        BY_TARGET,          // 按目标对象分组
        BY_DEPENDENCY,      // 按依赖关系分组
        BY_PRIORITY,        // 按优先级分组
        MIXED               // 混合策略
    };
    void setBatchGroupingStrategy(BatchGroupingStrategy strategy) { m_groupingStrategy = strategy; }
    BatchGroupingStrategy getBatchGroupingStrategy() const { return m_groupingStrategy; }

    // Rendering system integration
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }
    void setRenderingEngine(RenderingEngine* renderingEngine) { m_renderingEngine = renderingEngine; }
    void setOCCViewer(OCCViewer* occViewer) { m_occViewer = occViewer; }

    // Special update methods
    void scheduleGeometryRebuild(const std::string& geometryPath);
    void scheduleRenderingUpdate(const std::string& renderTarget = "");
    void scheduleLightingUpdate();
    void scheduleDisplayUpdate();

    // Emergency update (bypass batch processing)
    void executeImmediateUpdate(const UpdateTask& task);

private:
    UpdateCoordinator();
    ~UpdateCoordinator();
    UpdateCoordinator(const UpdateCoordinator&) = delete;
    UpdateCoordinator& operator=(const UpdateCoordinator&) = delete;

    // Internal data structures
    std::queue<UpdateTask> m_taskQueue;
    std::unordered_map<std::string, UpdateTask> m_pendingTasks;
    std::unordered_map<std::string, UpdateTask> m_executingTasks;
    std::unordered_map<std::string, BatchUpdateGroup> m_batchGroups;
    
    // Callback management
    std::unordered_map<int, UpdateCallback> m_updateCallbacks;
    std::unordered_map<int, BatchUpdateCallback> m_batchCallbacks;
    std::unordered_map<int, CompletionCallback> m_completionCallbacks;
    int m_nextCallbackId;

    // Thread management
    std::thread m_workerThread;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<bool> m_running;
    std::atomic<bool> m_executionPaused;

    // Configuration
    std::atomic<bool> m_batchProcessingEnabled;
    std::atomic<bool> m_smartBatchingEnabled;
    std::chrono::milliseconds m_batchTimeout;
    BatchGroupingStrategy m_groupingStrategy;

    // Performance statistics
    mutable std::mutex m_metricsMutex;
    PerformanceMetrics m_metrics;

    // External system references
    SceneManager* m_sceneManager;
    RenderingEngine* m_renderingEngine;
    OCCViewer* m_occViewer;

    // Internal methods
    void workerThreadFunction();
    void processTask(const UpdateTask& task);
    void processBatchGroup(BatchUpdateGroup& group);
    void executeTask(const UpdateTask& task);
    
    // Smart batch processing
    std::string createBatchGroup(const UpdateTask& task);
    void addTaskToBatchGroup(const UpdateTask& task, const std::string& groupId);
    bool shouldCreateNewBatchGroup(const UpdateTask& task, const std::string& groupId) const;
    void optimizeBatchGroup(BatchUpdateGroup& group);
    
    // Dependency handling
    bool areDependenciesSatisfied(const UpdateTask& task) const;
    void resolveDependencies(const std::string& taskId);
    
    // Task ID generation
    std::string generateTaskId();
    std::string generateBatchGroupId();
    
    // Performance monitoring
    void updatePerformanceMetrics(const UpdateTask& task, std::chrono::milliseconds executionTime);
    void recordBatchMetrics(const BatchUpdateGroup& group);
};

/**
 * @brief Update task builder
 * Provides fluent API for building update tasks
 */
class UpdateTaskBuilder {
public:
    UpdateTaskBuilder();
    ~UpdateTaskBuilder() = default;

    UpdateTaskBuilder& setType(UpdateTaskType type);
    UpdateTaskBuilder& setTargetPath(const std::string& path);
    UpdateTaskBuilder& setValues(const ParameterValue& oldValue, const ParameterValue& newValue);
    UpdateTaskBuilder& setPriority(int priority);
    UpdateTaskBuilder& setBatchable(bool batchable);
    UpdateTaskBuilder& setExecuteFunction(std::function<void()> func);
    UpdateTaskBuilder& addDependency(const std::string& dependencyTaskId);

    UpdateTask build();
    std::string submit();

private:
    UpdateTask m_task;
};

// Convenience macro definitions
#define SUBMIT_PARAM_CHANGE(path, oldVal, newVal) \
    UpdateCoordinator::getInstance().submitParameterChange(path, oldVal, newVal)

#define SUBMIT_IMMEDIATE_UPDATE(path, oldVal, newVal) \
    UpdateCoordinator::getInstance().submitParameterChange(path, oldVal, newVal, UpdateStrategy::IMMEDIATE)

#define SCHEDULE_GEOMETRY_REBUILD(path) \
    UpdateCoordinator::getInstance().scheduleGeometryRebuild(path)

#define SCHEDULE_RENDERING_UPDATE(target) \
    UpdateCoordinator::getInstance().scheduleRenderingUpdate(target)