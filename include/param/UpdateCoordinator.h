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
 * @brief 更新任务类型
 */
enum class UpdateTaskType {
    PARAMETER_CHANGE,      // 参数变更
    GEOMETRY_REBUILD,      // 几何重建
    RENDERING_UPDATE,      // 渲染更新
    LIGHTING_UPDATE,       // 光照更新
    DISPLAY_UPDATE,        // 显示更新
    PERFORMANCE_UPDATE,    // 性能更新
    BATCH_UPDATE          // 批量更新
};

/**
 * @brief 更新任务
 */
struct UpdateTask {
    UpdateTaskType type;
    std::string targetPath;                    // 目标参数路径
    ParameterValue oldValue;                   // 旧值
    ParameterValue newValue;                    // 新值
    std::chrono::steady_clock::time_point timestamp;
    int priority;                              // 优先级（0-10，10最高）
    std::unordered_set<std::string> dependencies; // 依赖的任务ID
    std::string taskId;                        // 任务唯一ID
    bool isBatchable;                          // 是否可批量处理
    std::function<void()> executeFunction;     // 执行函数
};

/**
 * @brief 批量更新组
 */
struct BatchUpdateGroup {
    std::string groupId;
    std::vector<UpdateTask> tasks;
    std::chrono::steady_clock::time_point createdTime;
    std::chrono::milliseconds maxWaitTime;
    bool isExecuting;
};

/**
 * @brief 更新策略
 */
enum class UpdateStrategy {
    IMMEDIATE,          // 立即执行
    BATCHED,            // 批量执行
    THROTTLED,          // 节流执行
    DEFERRED            // 延迟执行
};

/**
 * @brief 更新协调器
 * 智能协调参数变更和渲染更新，提供批量处理和依赖管理
 */
class UpdateCoordinator {
public:
    // 更新回调函数类型
    using UpdateCallback = std::function<void(const UpdateTask&)>;
    using BatchUpdateCallback = std::function<void(const BatchUpdateGroup&)>;
    using CompletionCallback = std::function<void(const std::string& taskId, bool success)>;

    static UpdateCoordinator& getInstance();

    // 初始化
    bool initialize();
    void shutdown();

    // 任务提交
    std::string submitUpdateTask(const UpdateTask& task);
    std::string submitParameterChange(
        const std::string& parameterPath,
        const ParameterValue& oldValue,
        const ParameterValue& newValue,
        UpdateStrategy strategy = UpdateStrategy::BATCHED
    );

    // 批量任务提交
    std::string submitBatchUpdate(
        const std::vector<UpdateTask>& tasks,
        const std::string& groupId = ""
    );

    // 任务管理
    bool cancelTask(const std::string& taskId);
    bool isTaskPending(const std::string& taskId) const;
    bool isTaskExecuting(const std::string& taskId) const;
    std::vector<std::string> getPendingTasks() const;
    std::vector<std::string> getExecutingTasks() const;

    // 批量处理控制
    void setBatchProcessingEnabled(bool enabled) { m_batchProcessingEnabled = enabled; }
    bool isBatchProcessingEnabled() const { return m_batchProcessingEnabled; }
    void setBatchTimeout(std::chrono::milliseconds timeout) { m_batchTimeout = timeout; }
    std::chrono::milliseconds getBatchTimeout() const { return m_batchTimeout; }

    // 依赖关系管理
    void addTaskDependency(const std::string& taskId, const std::string& dependencyTaskId);
    void removeTaskDependency(const std::string& taskId, const std::string& dependencyTaskId);
    std::vector<std::string> getTaskDependencies(const std::string& taskId) const;

    // 优先级管理
    void setTaskPriority(const std::string& taskId, int priority);
    int getTaskPriority(const std::string& taskId) const;

    // 回调注册
    int registerUpdateCallback(UpdateCallback callback);
    int registerBatchUpdateCallback(BatchUpdateCallback callback);
    int registerCompletionCallback(CompletionCallback callback);
    void unregisterCallback(int callbackId);

    // 执行控制
    void pauseExecution();
    void resumeExecution();
    bool isExecutionPaused() const { return m_executionPaused; }

    // 性能监控
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

    // 智能批量处理
    void enableSmartBatching(bool enabled) { m_smartBatchingEnabled = enabled; }
    bool isSmartBatchingEnabled() const { return m_smartBatchingEnabled; }
    
    // 批量分组策略
    enum class BatchGroupingStrategy {
        BY_TYPE,            // 按任务类型分组
        BY_TARGET,          // 按目标对象分组
        BY_DEPENDENCY,      // 按依赖关系分组
        BY_PRIORITY,        // 按优先级分组
        MIXED               // 混合策略
    };
    void setBatchGroupingStrategy(BatchGroupingStrategy strategy) { m_groupingStrategy = strategy; }
    BatchGroupingStrategy getBatchGroupingStrategy() const { return m_groupingStrategy; }

    // 渲染系统集成
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }
    void setRenderingEngine(RenderingEngine* renderingEngine) { m_renderingEngine = renderingEngine; }
    void setOCCViewer(OCCViewer* occViewer) { m_occViewer = occViewer; }

    // 特殊更新方法
    void scheduleGeometryRebuild(const std::string& geometryPath);
    void scheduleRenderingUpdate(const std::string& renderTarget = "");
    void scheduleLightingUpdate();
    void scheduleDisplayUpdate();

    // 紧急更新（绕过批量处理）
    void executeImmediateUpdate(const UpdateTask& task);

private:
    UpdateCoordinator();
    ~UpdateCoordinator();
    UpdateCoordinator(const UpdateCoordinator&) = delete;
    UpdateCoordinator& operator=(const UpdateCoordinator&) = delete;

    // 内部数据结构
    std::queue<UpdateTask> m_taskQueue;
    std::unordered_map<std::string, UpdateTask> m_pendingTasks;
    std::unordered_map<std::string, UpdateTask> m_executingTasks;
    std::unordered_map<std::string, BatchUpdateGroup> m_batchGroups;
    
    // 回调管理
    std::unordered_map<int, UpdateCallback> m_updateCallbacks;
    std::unordered_map<int, BatchUpdateCallback> m_batchCallbacks;
    std::unordered_map<int, CompletionCallback> m_completionCallbacks;
    int m_nextCallbackId;

    // 线程管理
    std::thread m_workerThread;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<bool> m_running;
    std::atomic<bool> m_executionPaused;

    // 配置
    std::atomic<bool> m_batchProcessingEnabled;
    std::atomic<bool> m_smartBatchingEnabled;
    std::chrono::milliseconds m_batchTimeout;
    BatchGroupingStrategy m_groupingStrategy;

    // 性能统计
    mutable std::mutex m_metricsMutex;
    PerformanceMetrics m_metrics;

    // 外部系统引用
    SceneManager* m_sceneManager;
    RenderingEngine* m_renderingEngine;
    OCCViewer* m_occViewer;

    // 内部方法
    void workerThreadFunction();
    void processTask(const UpdateTask& task);
    void processBatchGroup(BatchUpdateGroup& group);
    void executeTask(const UpdateTask& task);
    
    // 智能批量处理
    std::string createBatchGroup(const UpdateTask& task);
    void addTaskToBatchGroup(const UpdateTask& task, const std::string& groupId);
    bool shouldCreateNewBatchGroup(const UpdateTask& task, const std::string& groupId) const;
    void optimizeBatchGroup(BatchUpdateGroup& group);
    
    // 依赖关系处理
    bool areDependenciesSatisfied(const UpdateTask& task) const;
    void resolveDependencies(const std::string& taskId);
    
    // 任务ID生成
    std::string generateTaskId();
    std::string generateBatchGroupId();
    
    // 性能监控
    void updatePerformanceMetrics(const UpdateTask& task, std::chrono::milliseconds executionTime);
    void recordBatchMetrics(const BatchUpdateGroup& group);
};

/**
 * @brief 更新任务构建器
 * 提供流畅的API来构建更新任务
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

// 便利宏定义
#define SUBMIT_PARAM_CHANGE(path, oldVal, newVal) \
    UpdateCoordinator::getInstance().submitParameterChange(path, oldVal, newVal)

#define SUBMIT_IMMEDIATE_UPDATE(path, oldVal, newVal) \
    UpdateCoordinator::getInstance().submitParameterChange(path, oldVal, newVal, UpdateStrategy::IMMEDIATE)

#define SCHEDULE_GEOMETRY_REBUILD(path) \
    UpdateCoordinator::getInstance().scheduleGeometryRebuild(path)

#define SCHEDULE_RENDERING_UPDATE(target) \
    UpdateCoordinator::getInstance().scheduleRenderingUpdate(target)