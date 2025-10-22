# 异步引擎集成指南（中文）

## 快速集成到现有项目

### 第一步：初始化引擎

在主窗口类的头文件中添加：

```cpp
#include "async/AsyncEngineIntegration.h"

class MyMainFrame : public wxFrame {
private:
    std::unique_ptr<async::AsyncEngineIntegration> m_asyncEngine;
};
```

在构造函数中初始化：

```cpp
MyMainFrame::MyMainFrame() {
    // 创建异步引擎（自动检测CPU核心数）
    m_asyncEngine = std::make_unique<async::AsyncEngineIntegration>(this);
    
    // 绑定结果事件
    Bind(wxEVT_ASYNC_INTERSECTION_RESULT, &MyMainFrame::OnIntersectionResult, this);
    Bind(wxEVT_ASYNC_MESH_RESULT, &MyMainFrame::OnMeshResult, this);
    
    wxLogMessage("异步计算引擎已启动，工作线程数: %d", 
                 std::thread::hardware_concurrency() - 1);
}
```

### 第二步：提交计算任务

#### 计算边缘交点

```cpp
void MyMainFrame::ComputeIntersections(const TopoDS_Shape& shape) {
    // 生成唯一任务ID
    std::string taskId = "intersection_" + 
                         std::to_string(std::chrono::steady_clock::now()
                         .time_since_epoch().count());
    
    // 提交任务（非阻塞）
    m_asyncEngine->computeIntersectionsAsync(
        taskId,
        shape,
        1e-6  // 容差
    );
    
    // UI立即返回，继续响应用户操作
    wxLogMessage("已提交交点计算任务：%s", taskId);
}
```

#### 生成网格

```cpp
void MyMainFrame::GenerateMesh(const TopoDS_Shape& shape) {
    std::string taskId = "mesh_" + std::to_string(std::time(nullptr));
    
    m_asyncEngine->generateMeshAsync(
        taskId,
        shape,
        0.1,  // deflection
        0.5   // angle
    );
    
    // 显示进度提示
    m_statusBar->SetStatusText("正在生成网格...");
}
```

### 第三步：处理计算结果

#### 处理交点结果

```cpp
void MyMainFrame::OnIntersectionResult(async::AsyncIntersectionResultEvent& event) {
    const auto& result = event.GetResult();
    
    // 记录统计信息
    wxLogMessage("找到 %zu 个交点，耗时 %lld ms",
                 result.points.size(),
                 result.computeTime.count());
    
    // 渲染交点
    if (!result.points.empty()) {
        RenderIntersectionPoints(result.points);
        m_canvas->Refresh();
    }
    
    // 更新状态栏
    m_statusBar->SetStatusText(
        wxString::Format("交点计算完成：%zu 个点", result.points.size())
    );
}
```

#### 处理网格结果

```cpp
void MyMainFrame::OnMeshResult(async::AsyncMeshResultEvent& event) {
    auto meshData = event.GetMeshData();  // std::shared_ptr，零拷贝
    
    if (meshData) {
        // 打印网格信息
        wxLogMessage("网格生成完成：");
        wxLogMessage("  顶点数: %zu", meshData->vertexCount);
        wxLogMessage("  三角形数: %zu", meshData->triangleCount);
        wxLogMessage("  内存占用: %zu KB", meshData->getMemoryUsage() / 1024);
        
        // 上传到GPU进行渲染
        UploadMeshToGPU(
            meshData->vertices.data(),
            meshData->normals.data(),
            meshData->indices.data(),
            meshData->vertexCount,
            meshData->triangleCount
        );
        
        // 网格数据保留在共享缓存中，可以重复使用
        m_canvas->Refresh();
    }
}
```

### 第四步：重用缓存数据（可选）

```cpp
void MyMainFrame::RenderCachedMesh(const std::string& taskId) {
    // 从共享缓存获取之前计算的网格（零拷贝）
    std::string cacheKey = taskId + "_mesh";
    auto cachedMesh = m_asyncEngine->getSharedData<async::MeshData>(cacheKey);
    
    if (cachedMesh && cachedMesh->ready) {
        // 直接使用缓存的网格数据，无需重新计算
        RenderMesh(*cachedMesh->data);
        wxLogMessage("使用缓存的网格数据");
    } else {
        wxLogMessage("网格数据不在缓存中");
    }
}
```

## 常见使用场景

### 场景1：大模型加载时计算交点

```cpp
void MyMainFrame::OnFileOpen(wxCommandEvent& event) {
    wxFileDialog dialog(this, "打开STEP文件", "", "",
                       "STEP files (*.step;*.stp)|*.step;*.stp",
                       wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_OK) {
        // 加载模型
        TopoDS_Shape shape = LoadSTEPFile(dialog.GetPath().ToStdString());
        
        // 立即显示模型
        DisplayShape(shape);
        
        // 后台计算交点（不阻塞UI）
        m_asyncEngine->computeIntersectionsAsync(
            "main_intersection",
            shape,
            1e-6
        );
        
        // UI继续响应，用户可以旋转、缩放视图
    }
}
```

### 场景2：批量处理多个模型

```cpp
void MyMainFrame::ProcessMultipleShapes(const std::vector<TopoDS_Shape>& shapes) {
    m_totalTasks = shapes.size();
    m_completedTasks = 0;
    
    // 提交所有任务到异步引擎
    for (size_t i = 0; i < shapes.size(); ++i) {
        std::string taskId = "batch_" + std::to_string(i);
        m_asyncEngine->computeIntersectionsAsync(taskId, shapes[i], 1e-6);
    }
    
    wxLogMessage("已提交 %zu 个批处理任务", shapes.size());
}

void MyMainFrame::OnIntersectionResult(async::AsyncIntersectionResultEvent& event) {
    m_completedTasks++;
    
    // 更新进度
    double progress = (double)m_completedTasks / m_totalTasks * 100.0;
    m_progressBar->SetValue((int)progress);
    
    if (m_completedTasks == m_totalTasks) {
        wxLogMessage("批处理完成！");
        wxMessageBox("所有模型处理完成", "完成", wxICON_INFORMATION);
    }
}
```

### 场景3：交互式参数调整

```cpp
void MyMainFrame::OnToleranceChanged(wxSpinDoubleEvent& event) {
    double tolerance = event.GetValue();
    
    // 取消之前的计算任务
    m_asyncEngine->cancelTask("interactive_intersection");
    
    // 提交新的计算任务
    m_asyncEngine->computeIntersectionsAsync(
        "interactive_intersection",
        m_currentShape,
        tolerance
    );
    
    wxLogMessage("正在重新计算交点（容差=%.6f）...", tolerance);
}
```

### 场景4：预计算和缓存

```cpp
void MyMainFrame::PrecomputeForAllViewAngles(const TopoDS_Shape& shape) {
    // 为多个视角预生成网格
    std::vector<std::pair<double, double>> viewAngles = {
        {0.1, 0.5},   // 高质量
        {0.5, 1.0},   // 中等质量
        {1.0, 2.0}    // 低质量（用于快速预览）
    };
    
    for (size_t i = 0; i < viewAngles.size(); ++i) {
        std::string taskId = "precompute_lod" + std::to_string(i);
        m_asyncEngine->generateMeshAsync(
            taskId,
            shape,
            viewAngles[i].first,
            viewAngles[i].second
        );
    }
    
    wxLogMessage("预计算 %zu 个LOD级别", viewAngles.size());
}

void MyMainFrame::RenderWithAutoLOD(double cameraDistance) {
    // 根据相机距离选择合适的LOD
    int lodLevel = SelectLODLevel(cameraDistance);
    std::string cacheKey = "precompute_lod" + std::to_string(lodLevel) + "_mesh";
    
    auto mesh = m_asyncEngine->getSharedData<async::MeshData>(cacheKey);
    if (mesh && mesh->ready) {
        RenderMesh(*mesh->data);  // 零拷贝，即时渲染
    }
}
```

## 高级功能

### 任务取消

```cpp
// 取消特定任务
m_asyncEngine->cancelTask("task_id_123");

// 取消所有任务（例如用户点击"停止"按钮）
m_asyncEngine->cancelAllTasks();

// 窗口关闭时取消所有任务
void MyMainFrame::OnClose(wxCloseEvent& event) {
    m_asyncEngine->cancelAllTasks();
    event.Skip();
}
```

### 监控引擎状态

```cpp
void MyMainFrame::ShowEngineStatistics() {
    auto stats = m_asyncEngine->getStatistics();
    
    wxString msg;
    msg << "异步引擎统计:\n";
    msg << "队列中的任务: " << stats.queuedTasks << "\n";
    msg << "正在执行: " << stats.runningTasks << "\n";
    msg << "已完成: " << stats.completedTasks << "\n";
    msg << "失败: " << stats.failedTasks << "\n";
    msg << "平均执行时间: " << stats.avgExecutionTimeMs << " ms\n";
    msg << "总处理任务数: " << stats.totalProcessedTasks << "\n";
    
    wxMessageBox(msg, "引擎统计", wxICON_INFORMATION);
}

// 定时更新状态栏
void MyMainFrame::UpdateStatusBar() {
    auto stats = m_asyncEngine->getStatistics();
    m_statusBar->SetStatusText(
        wxString::Format("活动任务: %zu | 队列: %zu",
                        stats.runningTasks,
                        stats.queuedTasks),
        1  // 状态栏第二个区域
    );
}
```

### 自定义任务类型

```cpp
// 定义自定义输入/输出
struct MyCustomInput {
    TopoDS_Shape shape;
    double parameter1;
    std::string parameter2;
};

struct MyCustomResult {
    std::vector<double> computedValues;
    std::string statusMessage;
};

// 创建自定义任务
void MyMainFrame::SubmitCustomTask(const MyCustomInput& input) {
    using namespace async;
    
    auto task = std::make_shared<AsyncTask<MyCustomInput, MyCustomResult>>(
        "custom_task_" + std::to_string(std::time(nullptr)),
        input,
        // 计算函数（在工作线程中执行）
        [](const MyCustomInput& input, std::atomic<bool>& cancelled) -> MyCustomResult {
            MyCustomResult result;
            
            // 执行自定义计算
            for (int i = 0; i < 1000; ++i) {
                if (cancelled.load()) {
                    result.statusMessage = "Cancelled";
                    break;
                }
                
                // 模拟计算
                result.computedValues.push_back(performComputation(input, i));
            }
            
            result.statusMessage = "Completed";
            return result;
        },
        // 完成回调（在工作线程中执行）
        [this](const ComputeResult<MyCustomResult>& result) {
            if (result.success) {
                // 发送自定义事件到主线程
                wxQueueEvent(this, new MyCustomResultEvent(result.data));
            }
        }
    );
    
    // 设置进度回调（可选）
    task->setProgressCallback([this](int progress, const std::string& message) {
        wxQueueEvent(this, new ProgressEvent(progress, message));
    });
    
    // 提交到引擎
    m_asyncEngine->getEngine()->submitTask(task);
}
```

## 性能优化技巧

### 1. 合理设置任务粒度

```cpp
// 不好：任务太小，调度开销大
for (const auto& edge : edges) {
    m_asyncEngine->computeSomething(edge);  // 成百上千个小任务
}

// 好：批量处理
m_asyncEngine->computeBatch(edges);  // 一个大任务
```

### 2. 使用优先级

```cpp
// 用户交互相关的任务使用高优先级
AsyncTask<Input, Result>::Config config;
config.priority = TaskPriority::High;

// 后台预计算使用低优先级
config.priority = TaskPriority::Low;
```

### 3. 清理不需要的缓存

```cpp
// 手动清理大的缓存数据
m_asyncEngine->getEngine()->removeSharedData("old_large_mesh");

// 定期清理
void MyMainFrame::CleanupOldCache() {
    for (const auto& oldTaskId : m_oldTaskIds) {
        m_asyncEngine->getEngine()->removeSharedData(oldTaskId + "_mesh");
    }
    m_oldTaskIds.clear();
}
```

## 故障排查

### 问题：任务不执行

**检查项：**
```cpp
// 1. 确认引擎正在运行
if (!m_asyncEngine->getEngine()->isRunning()) {
    wxLogError("异步引擎未运行！");
}

// 2. 检查队列是否已满
auto stats = m_asyncEngine->getStatistics();
if (stats.queuedTasks >= maxQueueSize) {
    wxLogError("任务队列已满！");
}

// 3. 查看日志输出
// 在日志中搜索 "AsyncComputeEngine" 相关消息
```

### 问题：内存占用过高

**解决方法：**
```cpp
// 1. 及时清理缓存
m_asyncEngine->getEngine()->removeSharedData("large_data_key");

// 2. 限制缓存大小（在初始化时）
AsyncComputeEngine::Config config;
config.maxCacheSize = 50;  // 限制缓存条目数
m_engine = std::make_unique<AsyncComputeEngine>(config);
```

### 问题：UI仍然卡顿

**可能原因：**
```cpp
// 1. 在UI线程中做了重计算
// 不好：
void MyFrame::OnResult(ResultEvent& event) {
    auto points = event.GetPoints();
    for (auto& p : points) {
        heavyProcessing(p);  // ❌ 阻塞UI
    }
}

// 好：创建另一个异步任务处理
void MyFrame::OnResult(ResultEvent& event) {
    auto points = event.GetPoints();
    submitAnotherAsyncTask(points);  // ✓ 继续异步
}
```

## 完整示例程序

查看 `src/async/AsyncEngineExample.cpp` 了解完整的工作示例。

运行示例程序：
```bash
# 构建项目
cmake --build build --config Release

# 运行示例（如果已编译）
./build/Release/AsyncEngineExample.exe
```

## 总结

异步计算引擎提供：

✅ **简单易用** - 几行代码即可集成  
✅ **零拷贝** - 高效的内存共享  
✅ **线程安全** - 完善的同步机制  
✅ **灵活扩展** - 支持自定义任务  
✅ **性能优异** - 充分利用多核CPU  

立即开始使用，让您的应用程序更加流畅！

## 参考资料

- [完整文档](./ASYNC_COMPUTE_ENGINE.md) - 详细的架构说明
- [快速入门](./ASYNC_ENGINE_QUICK_START.md) - 5分钟上手
- [实施总结](./ASYNC_ENGINE_IMPLEMENTATION_SUMMARY.md) - 技术细节

如有问题，请参考日志输出或查看源代码中的注释。


