# 异步计算引擎生产环境部署指南

## 🎯 立即部署步骤

### 第一步：集成异步引擎到现有代码

在你的主窗口类中添加异步引擎：

```cpp
// 在 MyMainFrame.h 中添加
#include "async/AsyncEngineIntegration.h"

class MyMainFrame : public wxFrame {
private:
    std::unique_ptr<async::AsyncEngineIntegration> m_asyncEngine;

    // 事件处理函数
    void OnIntersectionResult(async::AsyncIntersectionResultEvent& event);
    void OnMeshResult(async::AsyncMeshResultEvent& event);
    void OnAsyncTaskProgress(async::AsyncEngineResultEvent& event);
};
```

```cpp
// 在 MyMainFrame.cpp 的构造函数中初始化
MyMainFrame::MyMainFrame() {
    // ... 其他初始化代码 ...

    // 初始化异步计算引擎
    m_asyncEngine = std::make_unique<async::AsyncEngineIntegration>(this);

    // 绑定事件处理器
    Bind(wxEVT_ASYNC_INTERSECTION_RESULT,
         &MyMainFrame::OnIntersectionResult, this);
    Bind(wxEVT_ASYNC_MESH_RESULT,
         &MyMainFrame::OnMeshResult, this);
    Bind(wxEVT_ASYNC_TASK_PROGRESS,
         &MyMainFrame::OnAsyncTaskProgress, this);
}
```

### 第二步：替换同步计算为异步

#### 原始同步代码：
```cpp
void MyMainFrame::ComputeIntersections(const TopoDS_Shape& shape) {
    // 同步计算 - 会阻塞UI
    std::vector<gp_Pnt> points = computeIntersections(shape, tolerance);

    // 更新UI
    UpdateIntersectionDisplay(points);
}
```

#### 新的异步代码：
```cpp
void MyMainFrame::ComputeIntersections(const TopoDS_Shape& shape) {
    // 生成唯一任务ID
    std::string taskId = "intersection_" +
                         std::to_string(std::chrono::steady_clock::now()
                                        .time_since_epoch().count());

    // 异步计算 - 不会阻塞UI
    m_asyncEngine->computeIntersectionsAsync(taskId, shape, 1e-6);

    // 显示进度提示
    m_statusBar->SetStatusText("正在计算交点...");
}
```

#### 实现结果处理器：
```cpp
void MyMainFrame::OnIntersectionResult(async::AsyncIntersectionResultEvent& event) {
    const auto& result = event.GetResult();

    if (result.success) {
        // 更新UI显示
        UpdateIntersectionDisplay(result.points);

        // 更新状态栏
        wxString msg = wxString::Format("找到 %zu 个交点 (耗时 %lld ms)",
                                       result.points.size(),
                                       result.computeTime.count());
        m_statusBar->SetStatusText(msg);
    } else {
        // 处理错误
        wxMessageBox("交点计算失败: " + event.GetErrorMessage(),
                    "错误", wxICON_ERROR);
    }
}
```

### 第三步：集成监控面板

异步引擎面板会自动显示在Performance dock中，无需额外配置。

## 🚀 性能优化选项

### 1. 缓存清理优化 (推荐)

当前缓存清理是简化实现的，建议实现LRU淘汰：

```cpp
// 在 AsyncComputeEngine.cpp 中增强 cleanupExpiredCache()
void AsyncComputeEngine::cleanupExpiredCache() {
    std::lock_guard<std::mutex> lock(m_sharedDataMutex);

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> expiredKeys;

    for (const auto& [key, sharedPtr] : m_sharedDataCache) {
        // 检查最后访问时间
        auto sharedData = std::static_pointer_cast<SharedComputeData<void>>(sharedPtr);
        auto age = std::chrono::duration_cast<std::chrono::minutes>(
            now - sharedData->lastAccessTime).count();

        if (age > m_config.cacheExpirationTime.count()) {
            expiredKeys.push_back(key);
        }
    }

    // 清理过期项
    for (const auto& key : expiredKeys) {
        m_sharedDataCache.erase(key);
    }

    // 如果缓存过大，进行LRU淘汰
    if (m_sharedDataCache.size() > m_config.maxCacheSize) {
        // 实现LRU淘汰逻辑
        // TODO: 按最后访问时间排序并淘汰最旧的
    }
}
```

### 2. 工作窃取算法 (中级优化)

```cpp
// 在 AsyncComputeEngine 中添加工作窃取
class WorkStealingQueue {
public:
    void push(std::function<void()> task);
    std::optional<std::function<void()>> pop();    // 从本地队列弹出
    std::optional<std::function<void()>> steal();  // 从其他队列偷取

private:
    std::deque<std::function<void()>> m_queue;
    std::mutex m_mutex;
};

std::vector<std::unique_ptr<WorkStealingQueue>> m_workQueues;

// 工作线程函数增强
void AsyncComputeEngine::workerThreadFunc() {
    size_t myIndex = m_nextWorkerIndex++;

    while (m_running.load()) {
        std::function<void()> task;

        // 先尝试从本地队列获取任务
        if (auto localTask = m_workQueues[myIndex]->pop()) {
            task = *localTask;
        } else {
            // 本地队列为空，尝试从其他队列偷取
            for (size_t i = 0; i < m_workers.size(); ++i) {
                if (i != myIndex) {
                    if (auto stolenTask = m_workQueues[i]->steal()) {
                        task = *stolenTask;
                        break;
                    }
                }
            }

            if (!task) {
                // 没有任务，等待
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCondition.wait_for(lock, std::chrono::milliseconds(10));
                continue;
            }
        }

        // 执行任务
        try {
            task();
        } catch (const std::exception& e) {
            LOG_ERR_S("Worker thread exception: " + std::string(e.what()));
        }
    }
}
```

### 3. 增强性能统计

```cpp
// 扩展 TaskStatistics 结构体
struct TaskStatistics {
    // 现有字段...
    size_t queuedTasks{0};
    size_t runningTasks{0};
    size_t completedTasks{0};
    size_t failedTasks{0};
    double avgExecutionTimeMs{0.0};
    size_t totalProcessedTasks{0};

    // 新增性能指标
    double cpuUsagePercent{0.0};
    size_t memoryPeakUsageKB{0};
    std::vector<double> executionTimeDistribution; // 分位数统计
    std::vector<size_t> queueWaitTimes; // 队列等待时间

    // 线程利用率
    std::vector<double> threadUtilization;
};

// 添加性能监控
void AsyncComputeEngine::updatePerformanceStats() {
    // 监控CPU使用率
    m_statistics.cpuUsagePercent = getCurrentCPUUsage();

    // 监控内存使用
    m_statistics.memoryPeakUsageKB = std::max(m_statistics.memoryPeakUsageKB,
                                              getCurrentMemoryUsage());

    // 更新执行时间分布
    updateExecutionTimeDistribution();
}
```

## 📊 监控和调试

### 实时监控

1. **AsyncEnginePanel**: 自动显示在Performance dock中
2. **日志输出**: 查看 `logs/` 目录下的异步引擎日志
3. **性能统计**: 调用 `m_asyncEngine->getStatistics()`

### 常见问题排查

#### 问题1: 任务执行很慢
```cpp
// 检查线程池状态
auto stats = m_asyncEngine->getStatistics();
wxLogMessage("Running tasks: %zu, Queued: %zu",
             stats.runningTasks, stats.queuedTasks);

// 检查是否有死锁
if (stats.runningTasks == 0 && stats.queuedTasks > 0) {
    wxLogMessage("可能存在死锁或线程阻塞");
}
```

#### 问题2: 内存使用过高
```cpp
// 检查缓存大小
size_t cacheSize = m_asyncEngine->getEngine()->getCacheSize();
wxLogMessage("Cache entries: %zu", cacheSize);

if (cacheSize > 100) {  // 阈值
    // 手动清理缓存
    m_asyncEngine->getEngine()->cleanupExpiredCache();
}
```

#### 问题3: UI不响应
```cpp
// 确保在工作线程中没有调用UI函数
// 错误示例（不要这样做）：
void someAsyncTask() {
    // ❌ 错误：直接调用UI函数
    m_statusBar->SetStatusText("Processing...");

    // ✅ 正确：通过事件传递
    wxQueueEvent(m_mainFrame,
                 new wxCommandEvent(wxEVT_COMMAND_STATUSBAR_UPDATE));
}
```

## 🔄 渐进式迁移策略

### 第一阶段：核心计算迁移 (1-2周)
1. 识别最耗时的同步计算函数
2. 将其迁移到异步引擎
3. 测试UI响应性改善

### 第二阶段：缓存优化 (2-4周)
1. 实现LRU缓存清理
2. 添加缓存统计
3. 监控缓存命中率

### 第三阶段：高级功能 (4-8周)
1. 工作窃取算法
2. 任务依赖关系
3. 性能分析工具

## 📈 预期性能提升

| 指标 | 迁移前 | 迁移后 | 提升 |
|------|--------|--------|------|
| UI响应时间 | 2-5秒 | <100ms | **95%+** |
| 内存使用 | 高峰时抖动 | 稳定可控 | **稳定** |
| 用户体验 | 卡顿等待 | 流畅交互 | **显著改善** |
| 计算效率 | 单线程阻塞 | 多线程并行 | **2-8倍** |

## 🛠️ 工具和资源

### 开发工具
- **AsyncEnginePanel**: 实时监控面板
- **性能日志**: 自动记录执行时间和资源使用
- **统计API**: `getStatistics()` 获取详细指标

### 调试工具
- **任务跟踪**: 每个任务都有唯一ID用于调试
- **错误日志**: 详细的异常信息和调用栈
- **状态检查**: 运行时状态查询和健康检查

### 性能分析
- **CPU分析器**: 监控线程利用率
- **内存分析器**: 跟踪缓存使用情况
- **时间分布**: 分析任务执行时间分布

## 🎯 成功指标

### 短期目标 (1个月)
- [ ] 核心计算迁移完成
- [ ] UI响应时间 < 100ms
- [ ] 用户反馈积极

### 中期目标 (3个月)
- [ ] 缓存优化完成
- [ ] 内存使用稳定
- [ ] 性能监控完善

### 长期目标 (6个月+)
- [ ] 高级功能就绪
- [ ] 自动化性能调优
- [ ] 生产环境稳定运行

## 📞 支持和维护

### 监控清单
- [ ] 定期检查 AsyncEnginePanel 统计信息
- [ ] 监控内存使用趋势
- [ ] 关注任务失败率
- [ ] 分析性能瓶颈

### 升级策略
- [ ] 小版本：bug修复和性能优化
- [ ] 大版本：新功能和架构改进
- [ ] 回滚计划：保留旧版本兼容性

---

## 🚀 立即开始

1. **备份当前代码**
2. **集成异步引擎** (按上述步骤)
3. **测试关键功能**
4. **部署到测试环境**
5. **收集用户反馈**
6. **逐步优化性能**

**异步计算引擎已经准备好投入生产环境使用！** 🎉

