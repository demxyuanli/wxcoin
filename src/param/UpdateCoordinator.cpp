#include "param/UpdateCoordinator.h"
#include "logger/Logger.h"
#include <random>
#include <algorithm>
#include <chrono>

// UpdateCoordinator 实现
UpdateCoordinator& UpdateCoordinator::getInstance() {
    static UpdateCoordinator instance;
    return instance;
}

UpdateCoordinator::UpdateCoordinator()
    : m_nextCallbackId(1)
    , m_running(false)
    , m_executionPaused(false)
    , m_batchProcessingEnabled(true)
    , m_smartBatchingEnabled(true)
    , m_batchTimeout(std::chrono::milliseconds(100))
    , m_groupingStrategy(BatchGroupingStrategy::MIXED)
    , m_sceneManager(nullptr)
    , m_renderingEngine(nullptr)
    , m_occViewer(nullptr) {
    
    // 初始化性能统计
    m_metrics.totalTasksSubmitted = 0;
    m_metrics.totalTasksExecuted = 0;
    m_metrics.totalBatchGroups = 0;
    m_metrics.averageBatchSize = 0;
    m_metrics.averageExecutionTime = std::chrono::milliseconds(0);
    m_metrics.averageWaitTime = std::chrono::milliseconds(0);
    m_metrics.dependencyConflicts = 0;
    m_metrics.cancelledTasks = 0;
    
    LOG_INF_S("UpdateCoordinator: Initialized update coordinator");
}

UpdateCoordinator::~UpdateCoordinator() {
    shutdown();
}

bool UpdateCoordinator::initialize() {
    if (m_running) {
        LOG_WRN_S("UpdateCoordinator: Already initialized");
        return true;
    }
    
    m_running = true;
    m_workerThread = std::thread(&UpdateCoordinator::workerThreadFunction, this);
    
    LOG_INF_S("UpdateCoordinator: Initialized successfully");
    return true;
}

void UpdateCoordinator::shutdown() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    m_queueCondition.notify_all();
    
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    
    // 清理未完成的任务
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingTasks.clear();
        m_executingTasks.clear();
        m_batchGroups.clear();
    }
    
    LOG_INF_S("UpdateCoordinator: Shutdown completed");
}

std::string UpdateCoordinator::submitUpdateTask(const UpdateTask& task) {
    if (!m_running) {
        LOG_ERR_S("UpdateCoordinator: Not initialized");
        return "";
    }
    
    UpdateTask taskCopy = task;
    taskCopy.taskId = generateTaskId();
    taskCopy.timestamp = std::chrono::steady_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_pendingTasks[taskCopy.taskId] = taskCopy;
        m_queue.push(taskCopy);
        m_metrics.totalTasksSubmitted++;
    }
    
    m_queueCondition.notify_one();
    
    LOG_DBG_S("UpdateCoordinator: Submitted task " + taskCopy.taskId + " of type " + 
              std::to_string(static_cast<int>(taskCopy.type)));
    
    return taskCopy.taskId;
}

std::string UpdateCoordinator::submitParameterChange(
    const std::string& parameterPath,
    const ParameterValue& oldValue,
    const ParameterValue& newValue,
    UpdateStrategy strategy) {
    
    UpdateTask task;
    task.type = UpdateTaskType::PARAMETER_CHANGE;
    task.targetPath = parameterPath;
    task.oldValue = oldValue;
    task.newValue = newValue;
    task.priority = 5; // 默认优先级
    task.isBatchable = (strategy == UpdateStrategy::BATCHED);
    
    // 根据策略设置执行函数
    switch (strategy) {
        case UpdateStrategy::IMMEDIATE:
            task.executeFunction = [this, parameterPath, newValue]() {
                executeImmediateUpdate({UpdateTaskType::PARAMETER_CHANGE, parameterPath, {}, newValue, 
                                      std::chrono::steady_clock::now(), 10, {}, "", true, nullptr});
            };
            break;
        case UpdateStrategy::BATCHED:
            task.executeFunction = [this, parameterPath, newValue]() {
                // 批量更新逻辑
                LOG_DBG_S("UpdateCoordinator: Executing batched parameter change for " + parameterPath);
            };
            break;
        case UpdateStrategy::THROTTLED:
            task.executeFunction = [this, parameterPath, newValue]() {
                // 节流更新逻辑
                LOG_DBG_S("UpdateCoordinator: Executing throttled parameter change for " + parameterPath);
            };
            break;
        case UpdateStrategy::DEFERRED:
            task.executeFunction = [this, parameterPath, newValue]() {
                // 延迟更新逻辑
                LOG_DBG_S("UpdateCoordinator: Executing deferred parameter change for " + parameterPath);
            };
            break;
    }
    
    return submitUpdateTask(task);
}

std::string UpdateCoordinator::submitBatchUpdate(const std::vector<UpdateTask>& tasks, const std::string& groupId) {
    if (tasks.empty()) {
        LOG_WRN_S("UpdateCoordinator: Empty batch update submitted");
        return "";
    }
    
    std::string actualGroupId = groupId.empty() ? generateBatchGroupId() : groupId;
    
    BatchUpdateGroup group;
    group.groupId = actualGroupId;
    group.tasks = tasks;
    group.createdTime = std::chrono::steady_clock::now();
    group.maxWaitTime = m_batchTimeout;
    group.isExecuting = false;
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_batchGroups[actualGroupId] = group;
        m_metrics.totalBatchGroups++;
    }
    
    // 提交批量任务
    for (const auto& task : tasks) {
        UpdateTask taskCopy = task;
        taskCopy.taskId = generateTaskId();
        taskCopy.timestamp = std::chrono::steady_clock::now();
        taskCopy.isBatchable = true;
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_pendingTasks[taskCopy.taskId] = taskCopy;
            m_queue.push(taskCopy);
            m_metrics.totalTasksSubmitted++;
        }
    }
    
    m_queueCondition.notify_one();
    
    LOG_INF_S("UpdateCoordinator: Submitted batch update " + actualGroupId + " with " + 
              std::to_string(tasks.size()) + " tasks");
    
    return actualGroupId;
}

bool UpdateCoordinator::cancelTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto pendingIt = m_pendingTasks.find(taskId);
    if (pendingIt != m_pendingTasks.end()) {
        m_pendingTasks.erase(pendingIt);
        m_metrics.cancelledTasks++;
        
        // 从队列中移除
        std::queue<UpdateTask> newQueue;
        while (!m_queue.empty()) {
            UpdateTask task = m_queue.front();
            m_queue.pop();
            if (task.taskId != taskId) {
                newQueue.push(task);
            }
        }
        m_queue = newQueue;
        
        LOG_DBG_S("UpdateCoordinator: Cancelled task " + taskId);
        return true;
    }
    
    return false;
}

bool UpdateCoordinator::isTaskPending(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_pendingTasks.find(taskId) != m_pendingTasks.end();
}

bool UpdateCoordinator::isTaskExecuting(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_executingTasks.find(taskId) != m_executingTasks.end();
}

std::vector<std::string> UpdateCoordinator::getPendingTasks() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::vector<std::string> tasks;
    for (const auto& pair : m_pendingTasks) {
        tasks.push_back(pair.first);
    }
    return tasks;
}

std::vector<std::string> UpdateCoordinator::getExecutingTasks() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::vector<std::string> tasks;
    for (const auto& pair : m_executingTasks) {
        tasks.push_back(pair.first);
    }
    return tasks;
}

void UpdateCoordinator::addTaskDependency(const std::string& taskId, const std::string& dependencyTaskId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto it = m_pendingTasks.find(taskId);
    if (it != m_pendingTasks.end()) {
        it->second.dependencies.insert(dependencyTaskId);
        LOG_DBG_S("UpdateCoordinator: Added dependency " + dependencyTaskId + " to task " + taskId);
    }
}

void UpdateCoordinator::removeTaskDependency(const std::string& taskId, const std::string& dependencyTaskId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto it = m_pendingTasks.find(taskId);
    if (it != m_pendingTasks.end()) {
        it->second.dependencies.erase(dependencyTaskId);
        LOG_DBG_S("UpdateCoordinator: Removed dependency " + dependencyTaskId + " from task " + taskId);
    }
}

std::vector<std::string> UpdateCoordinator::getTaskDependencies(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto it = m_pendingTasks.find(taskId);
    if (it != m_pendingTasks.end()) {
        return std::vector<std::string>(it->second.dependencies.begin(), it->second.dependencies.end());
    }
    
    return {};
}

void UpdateCoordinator::setTaskPriority(const std::string& taskId, int priority) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto it = m_pendingTasks.find(taskId);
    if (it != m_pendingTasks.end()) {
        it->second.priority = priority;
        LOG_DBG_S("UpdateCoordinator: Set priority " + std::to_string(priority) + " for task " + taskId);
    }
}

int UpdateCoordinator::getTaskPriority(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto it = m_pendingTasks.find(taskId);
    if (it != m_pendingTasks.end()) {
        return it->second.priority;
    }
    
    return 0;
}

int UpdateCoordinator::registerUpdateCallback(UpdateCallback callback) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    int id = m_nextCallbackId++;
    m_updateCallbacks[id] = callback;
    LOG_DBG_S("UpdateCoordinator: Registered update callback with ID " + std::to_string(id));
    return id;
}

int UpdateCoordinator::registerBatchUpdateCallback(BatchUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    int id = m_nextCallbackId++;
    m_batchCallbacks[id] = callback;
    LOG_DBG_S("UpdateCoordinator: Registered batch update callback with ID " + std::to_string(id));
    return id;
}

int UpdateCoordinator::registerCompletionCallback(CompletionCallback callback) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    int id = m_nextCallbackId++;
    m_completionCallbacks[id] = callback;
    LOG_DBG_S("UpdateCoordinator: Registered completion callback with ID " + std::to_string(id));
    return id;
}

void UpdateCoordinator::unregisterCallback(int callbackId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto updateIt = m_updateCallbacks.find(callbackId);
    if (updateIt != m_updateCallbacks.end()) {
        m_updateCallbacks.erase(updateIt);
        LOG_DBG_S("UpdateCoordinator: Unregistered update callback with ID " + std::to_string(callbackId));
        return;
    }
    
    auto batchIt = m_batchCallbacks.find(callbackId);
    if (batchIt != m_batchCallbacks.end()) {
        m_batchCallbacks.erase(batchIt);
        LOG_DBG_S("UpdateCoordinator: Unregistered batch update callback with ID " + std::to_string(callbackId));
        return;
    }
    
    auto completionIt = m_completionCallbacks.find(callbackId);
    if (completionIt != m_completionCallbacks.end()) {
        m_completionCallbacks.erase(completionIt);
        LOG_DBG_S("UpdateCoordinator: Unregistered completion callback with ID " + std::to_string(callbackId));
        return;
    }
}

void UpdateCoordinator::pauseExecution() {
    m_executionPaused = true;
    LOG_INF_S("UpdateCoordinator: Execution paused");
}

void UpdateCoordinator::resumeExecution() {
    m_executionPaused = false;
    m_queueCondition.notify_all();
    LOG_INF_S("UpdateCoordinator: Execution resumed");
}

UpdateCoordinator::PerformanceMetrics UpdateCoordinator::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_metrics;
}

void UpdateCoordinator::resetPerformanceMetrics() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_metrics = PerformanceMetrics{};
    LOG_INF_S("UpdateCoordinator: Performance metrics reset");
}

void UpdateCoordinator::scheduleGeometryRebuild(const std::string& geometryPath) {
    UpdateTask task;
    task.type = UpdateTaskType::GEOMETRY_REBUILD;
    task.targetPath = geometryPath;
    task.priority = 8; // 高优先级
    task.isBatchable = false; // 几何重建通常不能批量处理
    
    task.executeFunction = [this, geometryPath]() {
        LOG_INF_S("UpdateCoordinator: Executing geometry rebuild for " + geometryPath);
        // 这里应该调用实际的几何重建逻辑
    };
    
    submitUpdateTask(task);
}

void UpdateCoordinator::scheduleRenderingUpdate(const std::string& renderTarget) {
    UpdateTask task;
    task.type = UpdateTaskType::RENDERING_UPDATE;
    task.targetPath = renderTarget;
    task.priority = 6;
    task.isBatchable = true;
    
    task.executeFunction = [this, renderTarget]() {
        LOG_DBG_S("UpdateCoordinator: Executing rendering update for " + renderTarget);
        // 这里应该调用实际的渲染更新逻辑
    };
    
    submitUpdateTask(task);
}

void UpdateCoordinator::scheduleLightingUpdate() {
    UpdateTask task;
    task.type = UpdateTaskType::LIGHTING_UPDATE;
    task.priority = 7;
    task.isBatchable = true;
    
    task.executeFunction = [this]() {
        LOG_DBG_S("UpdateCoordinator: Executing lighting update");
        // 这里应该调用实际的光照更新逻辑
    };
    
    submitUpdateTask(task);
}

void UpdateCoordinator::scheduleDisplayUpdate() {
    UpdateTask task;
    task.type = UpdateTaskType::DISPLAY_UPDATE;
    task.priority = 5;
    task.isBatchable = true;
    
    task.executeFunction = [this]() {
        LOG_DBG_S("UpdateCoordinator: Executing display update");
        // 这里应该调用实际的显示更新逻辑
    };
    
    submitUpdateTask(task);
}

void UpdateCoordinator::executeImmediateUpdate(const UpdateTask& task) {
    LOG_INF_S("UpdateCoordinator: Executing immediate update for task " + task.taskId);
    
    try {
        if (task.executeFunction) {
            task.executeFunction();
        }
        
        // 通知完成回调
        for (const auto& pair : m_completionCallbacks) {
            try {
                pair.second(task.taskId, true);
            } catch (const std::exception& e) {
                LOG_ERR_S("UpdateCoordinator: Completion callback failed: " + std::string(e.what()));
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("UpdateCoordinator: Immediate update execution failed: " + std::string(e.what()));
        
        // 通知失败回调
        for (const auto& pair : m_completionCallbacks) {
            try {
                pair.second(task.taskId, false);
            } catch (const std::exception& e2) {
                LOG_ERR_S("UpdateCoordinator: Completion callback failed: " + std::string(e2.what()));
            }
        }
    }
}

// 私有方法实现
void UpdateCoordinator::workerThreadFunction() {
    LOG_INF_S("UpdateCoordinator: Worker thread started");
    
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        
        // 等待任务或停止信号
        m_queueCondition.wait(lock, [this]() {
            return !m_running || (!m_queue.empty() && !m_executionPaused);
        });
        
        if (!m_running) {
            break;
        }
        
        if (m_executionPaused) {
            continue;
        }
        
        if (m_queue.empty()) {
            continue;
        }
        
        // 获取下一个任务
        UpdateTask task = m_queue.front();
        m_queue.pop();
        
        // 检查依赖关系
        if (!areDependenciesSatisfied(task)) {
            // 将任务放回队列末尾
            m_queue.push(task);
            continue;
        }
        
        // 移动到执行中列表
        m_pendingTasks.erase(task.taskId);
        m_executingTasks[task.taskId] = task;
        
        lock.unlock();
        
        // 处理任务
        processTask(task);
        
        // 从执行中列表移除
        {
            std::lock_guard<std::mutex> execLock(m_queueMutex);
            m_executingTasks.erase(task.taskId);
        }
    }
    
    LOG_INF_S("UpdateCoordinator: Worker thread stopped");
}

void UpdateCoordinator::processTask(const UpdateTask& task) {
    auto startTime = std::chrono::steady_clock::now();
    
    try {
        if (m_batchProcessingEnabled && task.isBatchable) {
            // 尝试批量处理
            std::string groupId = createBatchGroup(task);
            addTaskToBatchGroup(task, groupId);
            
            // 检查是否应该执行批量组
            auto it = m_batchGroups.find(groupId);
            if (it != m_batchGroups.end()) {
                auto& group = it->second;
                auto elapsed = std::chrono::steady_clock::now() - group.createdTime;
                
                if (elapsed >= group.maxWaitTime || group.tasks.size() >= 10) { // 最大批量大小
                    processBatchGroup(group);
                    m_batchGroups.erase(it);
                }
            }
        } else {
            // 立即执行
            executeTask(task);
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto executionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        updatePerformanceMetrics(task, executionTime);
        
        // 通知更新回调
        for (const auto& pair : m_updateCallbacks) {
            try {
                pair.second(task);
            } catch (const std::exception& e) {
                LOG_ERR_S("UpdateCoordinator: Update callback failed: " + std::string(e.what()));
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("UpdateCoordinator: Task processing failed: " + std::string(e.what()));
    }
}

void UpdateCoordinator::processBatchGroup(BatchUpdateGroup& group) {
    LOG_INF_S("UpdateCoordinator: Processing batch group " + group.groupId + " with " + 
              std::to_string(group.tasks.size()) + " tasks");
    
    group.isExecuting = true;
    
    try {
        // 优化批量组
        optimizeBatchGroup(group);
        
        // 执行批量任务
        for (const auto& task : group.tasks) {
            executeTask(task);
        }
        
        // 通知批量更新回调
        for (const auto& pair : m_batchCallbacks) {
            try {
                pair.second(group);
            } catch (const std::exception& e) {
                LOG_ERR_S("UpdateCoordinator: Batch update callback failed: " + std::string(e.what()));
            }
        }
        
        recordBatchMetrics(group);
        
    } catch (const std::exception& e) {
        LOG_ERR_S("UpdateCoordinator: Batch group processing failed: " + std::string(e.what()));
    }
    
    group.isExecuting = false;
}

void UpdateCoordinator::executeTask(const UpdateTask& task) {
    try {
        if (task.executeFunction) {
            task.executeFunction();
        }
        
        m_metrics.totalTasksExecuted++;
        
        // 通知完成回调
        for (const auto& pair : m_completionCallbacks) {
            try {
                pair.second(task.taskId, true);
            } catch (const std::exception& e) {
                LOG_ERR_S("UpdateCoordinator: Completion callback failed: " + std::string(e.what()));
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERR_S("UpdateCoordinator: Task execution failed: " + std::string(e.what()));
        
        // 通知失败回调
        for (const auto& pair : m_completionCallbacks) {
            try {
                pair.second(task.taskId, false);
            } catch (const std::exception& e2) {
                LOG_ERR_S("UpdateCoordinator: Completion callback failed: " + std::string(e2.what()));
            }
        }
    }
}

std::string UpdateCoordinator::createBatchGroup(const UpdateTask& task) {
    // 根据分组策略创建批量组
    std::string groupId;
    
    switch (m_groupingStrategy) {
        case BatchGroupingStrategy::BY_TYPE:
            groupId = "batch_" + std::to_string(static_cast<int>(task.type));
            break;
        case BatchGroupingStrategy::BY_TARGET:
            groupId = "batch_" + task.targetPath;
            break;
        case BatchGroupingStrategy::BY_PRIORITY:
            groupId = "batch_priority_" + std::to_string(task.priority);
            break;
        case BatchGroupingStrategy::MIXED:
        default:
            groupId = "batch_mixed_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count() / 1000000);
            break;
    }
    
    return groupId;
}

void UpdateCoordinator::addTaskToBatchGroup(const UpdateTask& task, const std::string& groupId) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    auto it = m_batchGroups.find(groupId);
    if (it != m_batchGroups.end()) {
        it->second.tasks.push_back(task);
    } else {
        BatchUpdateGroup group;
        group.groupId = groupId;
        group.tasks = {task};
        group.createdTime = std::chrono::steady_clock::now();
        group.maxWaitTime = m_batchTimeout;
        group.isExecuting = false;
        m_batchGroups[groupId] = group;
    }
}

bool UpdateCoordinator::shouldCreateNewBatchGroup(const UpdateTask& task, const std::string& groupId) const {
    auto it = m_batchGroups.find(groupId);
    if (it == m_batchGroups.end()) {
        return true;
    }
    
    const auto& group = it->second;
    auto elapsed = std::chrono::steady_clock::now() - group.createdTime;
    
    // 如果组已存在太久或任务太多，创建新组
    return elapsed >= group.maxWaitTime || group.tasks.size() >= 10;
}

void UpdateCoordinator::optimizeBatchGroup(BatchUpdateGroup& group) {
    // 按优先级排序
    std::sort(group.tasks.begin(), group.tasks.end(), 
              [](const UpdateTask& a, const UpdateTask& b) {
                  return a.priority > b.priority;
              });
    
    // 移除重复任务
    std::unordered_set<std::string> seenPaths;
    auto it = group.tasks.begin();
    while (it != group.tasks.end()) {
        if (seenPaths.find(it->targetPath) != seenPaths.end()) {
            it = group.tasks.erase(it);
        } else {
            seenPaths.insert(it->targetPath);
            ++it;
        }
    }
    
    LOG_DBG_S("UpdateCoordinator: Optimized batch group " + group.groupId + 
              " to " + std::to_string(group.tasks.size()) + " tasks");
}

bool UpdateCoordinator::areDependenciesSatisfied(const UpdateTask& task) const {
    for (const std::string& depId : task.dependencies) {
        // 检查依赖任务是否已完成
        if (m_pendingTasks.find(depId) != m_pendingTasks.end() ||
            m_executingTasks.find(depId) != m_executingTasks.end()) {
            return false;
        }
    }
    return true;
}

void UpdateCoordinator::resolveDependencies(const std::string& taskId) {
    // 这里可以实现依赖关系解析逻辑
    LOG_DBG_S("UpdateCoordinator: Resolving dependencies for task " + taskId);
}

std::string UpdateCoordinator::generateTaskId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return "task_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + 
           "_" + std::to_string(dis(gen));
}

std::string UpdateCoordinator::generateBatchGroupId() {
    return "batch_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
}

void UpdateCoordinator::updatePerformanceMetrics(const UpdateTask& task, std::chrono::milliseconds executionTime) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // 更新平均执行时间
    if (m_metrics.totalTasksExecuted > 0) {
        m_metrics.averageExecutionTime = std::chrono::milliseconds(
            (m_metrics.averageExecutionTime.count() * (m_metrics.totalTasksExecuted - 1) + executionTime.count()) / 
            m_metrics.totalTasksExecuted
        );
    } else {
        m_metrics.averageExecutionTime = executionTime;
    }
}

void UpdateCoordinator::recordBatchMetrics(const BatchUpdateGroup& group) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // 更新平均批量大小
    if (m_metrics.totalBatchGroups > 0) {
        m_metrics.averageBatchSize = (m_metrics.averageBatchSize * (m_metrics.totalBatchGroups - 1) + group.tasks.size()) / 
                                     m_metrics.totalBatchGroups;
    } else {
        m_metrics.averageBatchSize = group.tasks.size();
    }
}

// UpdateTaskBuilder 实现
UpdateTaskBuilder::UpdateTaskBuilder() {
    m_task.type = UpdateTaskType::PARAMETER_CHANGE;
    m_task.priority = 5;
    m_task.isBatchable = true;
    m_task.taskId = "";
}

UpdateTaskBuilder& UpdateTaskBuilder::setType(UpdateTaskType type) {
    m_task.type = type;
    return *this;
}

UpdateTaskBuilder& UpdateTaskBuilder::setTargetPath(const std::string& path) {
    m_task.targetPath = path;
    return *this;
}

UpdateTaskBuilder& UpdateTaskBuilder::setValues(const ParameterValue& oldValue, const ParameterValue& newValue) {
    m_task.oldValue = oldValue;
    m_task.newValue = newValue;
    return *this;
}

UpdateTaskBuilder& UpdateTaskBuilder::setPriority(int priority) {
    m_task.priority = priority;
    return *this;
}

UpdateTaskBuilder& UpdateTaskBuilder::setBatchable(bool batchable) {
    m_task.isBatchable = batchable;
    return *this;
}

UpdateTaskBuilder& UpdateTaskBuilder::setExecuteFunction(std::function<void()> func) {
    m_task.executeFunction = func;
    return *this;
}

UpdateTaskBuilder& UpdateTaskBuilder::addDependency(const std::string& dependencyTaskId) {
    m_task.dependencies.insert(dependencyTaskId);
    return *this;
}

UpdateTask UpdateTaskBuilder::build() {
    return m_task;
}

std::string UpdateTaskBuilder::submit() {
    return UpdateCoordinator::getInstance().submitUpdateTask(m_task);
}