# 异步交点计算系统使用指南

## 🎯 概述

异步交点计算系统允许在后台线程计算边线交点，不阻塞UI主线程，提供流畅的用户体验。

**关键特性：**
- ✅ 后台线程异步计算
- ✅ 实时进度更新（状态栏）
- ✅ 详细日志输出（Message面板）
- ✅ 自动缓存结果
- ✅ 完成后回调渲染
- ✅ 支持任务取消
- ✅ 线程安全

---

## 📦 组件架构

### 核心类

```
AsyncIntersectionTask          异步计算任务
    ↓
AsyncIntersectionManager      任务管理器（UI集成）
    ↓
FlatFrame事件系统             wxWidgets事件
```

### 文件结构

```
include/edges/
  ├── AsyncIntersectionTask.h        异步任务定义
  └── AsyncIntersectionManager.h     管理器定义

src/opencascade/edges/
  ├── AsyncIntersectionTask.cpp      异步任务实现
  └── AsyncIntersectionManager.cpp   管理器实现
```

---

## 🚀 快速开始

### 基础用法

```cpp
#include "edges/AsyncIntersectionManager.h"

// 1. 创建管理器（通常在窗口初始化时）
auto manager = std::make_shared<AsyncIntersectionManager>(
    frame,              // 主窗口
    statusBar,          // 状态栏（用于进度）
    messagePanel        // Message面板（用于日志）
);

// 2. 启动异步计算
manager->startIntersectionComputation(
    shape,              // TopoDS_Shape
    tolerance,          // 容差
    [this](const std::vector<gp_Pnt>& intersectionPoints) {
        // 完成回调 - 在主线程执行
        renderIntersectionNodes(intersectionPoints);
    }
);

// 3. 可选：取消计算
if (needCancel) {
    manager->cancelCurrentComputation();
}
```

---

## 📝 详细使用指南

### 1. AsyncIntersectionTask - 低级API

如果需要更细粒度的控制，可以直接使用`AsyncIntersectionTask`：

```cpp
#include "edges/AsyncIntersectionTask.h"

// 创建任务
auto task = std::make_shared<AsyncIntersectionTask>(
    shape,
    tolerance,
    frame,
    // 完成回调
    [](const std::vector<gp_Pnt>& points) {
        LOG_INF_S("Intersection computation completed: " + 
                  std::to_string(points.size()) + " points");
    },
    // 进度回调（可选）
    [](int progress, const std::string& message, const std::string& details) {
        LOG_INF_S("Progress: " + std::to_string(progress) + "% - " + message);
    },
    // 错误回调（可选）
    [](const std::string& error) {
        LOG_ERR_S("Intersection computation error: " + error);
    }
);

// 启动
if (task->start()) {
    LOG_INF_S("Task started successfully");
}

// 等待完成
task->waitForCompletion();  // 阻塞等待

// 或定时检查
while (task->isRunning()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int progress = task->getProgress();
    // 更新UI...
}
```

### 2. AsyncIntersectionManager - 高级API（推荐）

自动处理UI更新和事件：

```cpp
#include "edges/AsyncIntersectionManager.h"

// 在窗口类中添加成员
class MyFrame : public wxFrame {
private:
    std::shared_ptr<AsyncIntersectionManager> m_intersectionManager;
    
public:
    MyFrame() {
        // 初始化管理器
        m_intersectionManager = std::make_shared<AsyncIntersectionManager>(
            this,
            m_statusBar,
            m_messageOutput
        );
    }
    
    void computeIntersections(const TopoDS_Shape& shape, double tolerance) {
        // 检查是否已有任务运行
        if (m_intersectionManager->isComputationRunning()) {
            wxMessageBox("Intersection computation is already running. "
                        "Please wait or cancel the current task.",
                        "Busy", wxOK | wxICON_INFORMATION);
            return;
        }
        
        // 启动异步计算
        bool started = m_intersectionManager->startIntersectionComputation(
            shape,
            tolerance,
            [this](const std::vector<gp_Pnt>& points) {
                // 完成回调 - 自动在主线程执行
                onIntersectionComputationCompleted(points);
            }
        );
        
        if (!started) {
            wxMessageBox("Failed to start intersection computation.",
                        "Error", wxOK | wxICON_ERROR);
        }
    }
    
    void onIntersectionComputationCompleted(const std::vector<gp_Pnt>& points) {
        // 渲染交点节点
        for (const auto& pt : points) {
            renderIntersectionNode(pt);
        }
        
        // 刷新显示
        m_viewer->update();
        
        // 通知用户
        wxMessageBox(wxString::Format("Found %zu intersection points", points.size()),
                    "Completed", wxOK | wxICON_INFORMATION);
    }
    
    void onCancelButtonClicked() {
        m_intersectionManager->cancelCurrentComputation();
    }
};
```

---

## 🎨 UI集成

### 状态栏进度显示

管理器自动更新状态栏进度：

```
[▓▓▓▓░░░░░░ 40%] Computing intersections...
```

状态栏需要支持`SetProgress(int, wxString)`方法。

### Message面板输出

详细日志自动输出到Message面板：

```
[15:23:45] ========================================
[15:23:45] Starting Asynchronous Intersection Computation
[15:23:45] ========================================
[15:23:45] Tolerance: 0.005000
[15:23:45] Status: Initializing...
[15:23:45] Progress: 0%
[15:23:46] Progress: 5%
[15:23:46]   Phase 1/3: Extracting edges from CAD geometry
[15:23:47] Progress: 20%
[15:23:47]   Phase 1/3: Extracting edges
[15:23:47]     - Extracted 1234 edges
[15:23:48] Progress: 35%
[15:23:48]   Phase 3/3: Computing edge intersections
[15:23:48]     - Progress: 0% of intersection computation
[15:23:48]     - Using BVH spatial acceleration
[15:23:51] Progress: 95%
[15:23:51]   Phase 3/3: Computing edge intersections
[15:23:51]     - Progress: 60% of intersection computation
[15:23:52] Progress: 100%
[15:23:52]   Intersection computation completed successfully
[15:23:52]     - Found 234 intersection points
[15:23:52]     - Computation time: 7.2 seconds
[15:23:52]     - Result cached for future use
[15:23:52] ========================================
[15:23:52] Intersection Computation COMPLETED
[15:23:52] ========================================
[15:23:52] Result: 234 intersection points found
[15:23:52] Status: Success
[15:23:52] Cache: Result cached for future use
[15:23:52] ========================================
```

---

## 🔧 高级特性

### 1. 取消计算

```cpp
// 方法1：通过管理器取消
manager->cancelCurrentComputation();

// 方法2：通过任务取消
task->cancel();

// 任务会在下一个检查点安全退出
```

### 2. 等待完成

```cpp
// 无限等待
task->waitForCompletion();

// 超时等待（毫秒）
bool completed = task->waitForCompletion(5000);  // 5秒超时
if (!completed) {
    LOG_WRN_S("Task did not complete within 5 seconds");
}
```

### 3. 查询状态

```cpp
// 检查是否运行中
if (task->isRunning()) {
    LOG_INF_S("Task is still running");
}

// 检查是否被取消
if (task->isCancelled()) {
    LOG_INF_S("Task was cancelled");
}

// 获取当前进度
int progress = task->getProgress();  // 0-100

// 获取当前状态消息
std::string msg = task->getCurrentMessage();
```

### 4. 自定义进度回调

```cpp
auto task = std::make_shared<AsyncIntersectionTask>(
    shape, tolerance, frame,
    onComplete,
    // 自定义进度回调
    [this](int progress, const std::string& message, const std::string& details) {
        // 更新自定义UI
        m_progressBar->SetValue(progress);
        m_statusLabel->SetLabel(message);
        m_detailsText->SetValue(details);
        
        // 记录日志
        LOG_INF_S("Intersection computation: " + std::to_string(progress) + "%");
    }
);
```

---

## 🎯 最佳实践

### 1. 在ShowOriginalEdgesListener中集成

```cpp
#include "edges/AsyncIntersectionManager.h"

class ShowOriginalEdgesListener : public CommandListener {
private:
    OCCViewer* m_viewer;
    wxFrame* m_frame;
    std::shared_ptr<AsyncIntersectionManager> m_intersectionManager;
    
public:
    ShowOriginalEdgesListener(OCCViewer* viewer, wxFrame* frame)
        : m_viewer(viewer), m_frame(frame) 
    {
        // 创建管理器
        FlatFrame* flatFrame = dynamic_cast<FlatFrame*>(frame);
        if (flatFrame) {
            m_intersectionManager = std::make_shared<AsyncIntersectionManager>(
                frame,
                flatFrame->GetFlatUIStatusBar(),
                flatFrame->getMessageOutput()
            );
        }
    }
    
    CommandResult executeCommand(...) {
        // ... 获取参数 ...
        
        if (highlightIntersectionNodes) {
            // 启动异步计算
            auto geometries = m_viewer->getAllGeometry();
            for (const auto& geom : geometries) {
                if (!geom) continue;
                
                m_intersectionManager->startIntersectionComputation(
                    geom->getShape(),
                    0.005,  // tolerance
                    [this, geom, nodeColor, nodeSize, nodeShape](
                        const std::vector<gp_Pnt>& points) {
                        // 渲染完成回调
                        geom->renderIntersectionNodes(
                            points, nodeColor, nodeSize, nodeShape);
                    }
                );
            }
        }
        
        return CommandResult(true, "Async intersection computation started");
    }
};
```

### 2. 避免重复计算

```cpp
// 检查是否已有任务运行
if (m_intersectionManager->isComputationRunning()) {
    wxMessageBox("Please wait for current computation to finish",
                "Busy", wxOK | wxICON_INFORMATION);
    return;
}

// 启动新计算
m_intersectionManager->startIntersectionComputation(...);
```

### 3. 资源清理

```cpp
class MyFrame : public wxFrame {
public:
    ~MyFrame() {
        // 取消所有运行中的任务
        if (m_intersectionManager) {
            m_intersectionManager->cancelCurrentComputation();
        }
        
        // 管理器会自动等待任务完成
    }
};
```

---

## 📊 性能优势

### 对比：同步 vs 异步

**同步计算（优化前）：**
```
用户点击"显示交点"
    ↓
UI冻结 ⚠️
    ↓
计算交点（5秒）⏰
    ↓
UI恢复
    ↓
显示结果
```

**异步计算（优化后）：**
```
用户点击"显示交点"
    ↓
立即返回 ✅
    ↓
UI保持响应 ✅
    ↓
后台计算（5秒）⏰
    │
    ├→ 实时进度更新 ⚡
    ├→ Message面板日志 📝
    ├→ 用户可取消 🛑
    │
    ↓
计算完成 → 自动渲染 ✅
```

### 用户体验改善

| 方面 | 同步 | 异步 | 改善 |
|------|------|------|------|
| **UI响应** | ❌ 冻结 | ✅ 流畅 | 无限好 |
| **进度反馈** | ❌ 无 | ✅ 实时 | 极大 |
| **可取消性** | ❌ 不可 | ✅ 可取消 | 重要 |
| **日志详情** | ⚠️ 事后 | ✅ 实时 | 很好 |
| **多任务** | ❌ 串行 | ✅ 可并行 | 好 |

---

## 🐛 故障排查

### 问题1：事件未触发

**症状：** 进度不更新，完成回调不执行

**原因：** wxWidgets事件未正确连接

**解决：**
```cpp
// 确保在主线程创建管理器
m_intersectionManager = std::make_shared<AsyncIntersectionManager>(
    this,  // 必须是有效的wxFrame*
    statusBar,
    messagePanel
);

// 检查frame指针
if (!frame) {
    LOG_ERR_S("Frame pointer is null!");
}
```

### 问题2：内存访问错误

**症状：** 程序崩溃，访问已释放的内存

**原因：** 回调中使用了已销毁的对象

**解决：**
```cpp
// 使用shared_ptr延长生命周期
auto geom = m_viewer->getGeometry();
auto geomPtr = std::shared_ptr<OCCGeometry>(geom, [](auto*){}); // 不delete

m_intersectionManager->startIntersectionComputation(
    shape, tolerance,
    [geomPtr](const std::vector<gp_Pnt>& points) {
        // geomPtr在回调执行前一定有效
        geomPtr->renderIntersectionNodes(points);
    }
);
```

### 问题3：任务无法取消

**症状：** 调用cancel后任务继续运行

**原因：** 计算循环中未检查取消标志

**解决：** 已在`AsyncIntersectionTask::computeIntersections()`中添加检查点

---

## 📚 API参考

### AsyncIntersectionTask

```cpp
class AsyncIntersectionTask {
public:
    // 构造函数
    AsyncIntersectionTask(
        const TopoDS_Shape& shape,
        double tolerance,
        wxFrame* frame,
        CompletionCallback onComplete,
        ProgressCallback onProgress = nullptr,
        ErrorCallback onError = nullptr);
    
    // 启动/控制
    bool start();
    void cancel();
    bool waitForCompletion(int timeoutMs = 0);
    
    // 查询
    bool isRunning() const;
    bool isCancelled() const;
    int getProgress() const;
    std::string getCurrentMessage() const;
};
```

### AsyncIntersectionManager

```cpp
class AsyncIntersectionManager {
public:
    // 构造函数
    AsyncIntersectionManager(
        wxFrame* frame,
        FlatUIStatusBar* statusBar = nullptr,
        wxTextCtrl* messagePanel = nullptr);
    
    // 计算控制
    bool startIntersectionComputation(
        const TopoDS_Shape& shape,
        double tolerance,
        AsyncIntersectionTask::CompletionCallback onComplete);
    
    void cancelCurrentComputation();
    bool isComputationRunning() const;
    
    // 配置
    void setStatusBar(FlatUIStatusBar* statusBar);
    void setMessagePanel(wxTextCtrl* messagePanel);
};
```

### 事件类型

```cpp
// 自定义wxWidgets事件
wxDECLARE_EVENT(wxEVT_INTERSECTION_COMPLETED, IntersectionCompletedEvent);
wxDECLARE_EVENT(wxEVT_INTERSECTION_ERROR, IntersectionErrorEvent);
wxDECLARE_EVENT(wxEVT_INTERSECTION_PROGRESS, IntersectionProgressEvent);
```

---

## ✅ 总结

### 何时使用异步计算

✅ **适用场景：**
- 交点计算耗时>1秒
- 需要实时进度反馈
- 用户可能需要取消操作
- 需要保持UI响应

❌ **不适用场景：**
- 计算耗时<100ms
- 简单的一次性计算
- 不需要进度反馈

### 迁移清单

从同步到异步的迁移步骤：

- [ ] 添加AsyncIntersectionManager成员到窗口类
- [ ] 初始化管理器（传入frame、statusBar、messagePanel）
- [ ] 替换同步调用为startIntersectionComputation()
- [ ] 将渲染逻辑移到完成回调
- [ ] 添加取消按钮（可选）
- [ ] 测试多次调用和取消逻辑

---

**文档版本:** 1.0  
**创建日期:** 2025-10-20  
**状态:** ✅ 实施完成  
**建议:** 🚀 **大幅提升用户体验的关键优化**



