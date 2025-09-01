# Canvas 性能问题深度分析

## 发现的关键问题

### 1. 直接渲染调用绕过了 wxWidgets 绘制系统
**位置**: `ViewRefreshManager::performRefresh` (第 83 行)
```cpp
m_canvas->render(false);  // 直接调用渲染，而不是 Refresh()
```
**问题**: 这会导致：
- 渲染调用不经过 wxWidgets 的绘制事件队列
- 可能与 `onPaint` 事件处理冲突
- 导致重复渲染或渲染时序问题

### 2. 双重渲染问题
**Canvas::onPaint**:
```cpp
void Canvas::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    render(false);  // 第一次渲染
    if (m_eventCoordinator) {
        m_eventCoordinator->handlePaintEvent(event);
    }
    event.Skip();  // 可能触发父窗口的绘制
}
```
同时，`ViewRefreshManager` 也可能调用 `render()`，导致双重渲染。

### 3. OpenGL 上下文频繁切换
**RenderingEngine::renderWithoutSwap**:
```cpp
if (!m_canvas->SetCurrent(*m_glContext)) {  // 每次渲染都设置上下文
```
频繁的 `SetCurrent` 调用会有性能开销，特别是在 reparenting 后。

### 4. 鼠标移动时的悬停检测
**Canvas::onMouseEvent**:
```cpp
if (event.GetEventType() == wxEVT_MOTION && m_occViewer) {
    wxPoint screenPos = event.GetPosition();
    m_occViewer->updateHoverSilhouetteAt(screenPos);  // 每次鼠标移动都更新
}
```
这可能触发额外的渲染或计算。

### 5. 进度定时器的影响
定时器每 50ms 运行一次，虽然不是主要问题，但在性能已经受限时可能加重负担。

## 性能问题的根本原因

1. **渲染管线混乱**: 存在多个渲染触发路径：
   - wxPaintEvent -> onPaint -> render()
   - ViewRefreshManager -> performRefresh -> render()
   - 鼠标事件 -> updateHoverSilhouetteAt -> 可能的渲染

2. **事件传播问题**: Canvas 的 `event.Skip()` 导致事件继续传播，可能触发父窗口的处理。

3. **OpenGL 上下文管理**: Canvas 被 reparent 到 DockWidget 后，OpenGL 上下文的管理可能受到影响。

## 解决方案

### 1. 修复 ViewRefreshManager
将直接的 `render()` 调用改为 `Refresh()`:
```cpp
// m_canvas->render(false);  // 旧代码
m_canvas->Refresh(false);     // 新代码
```

### 2. 优化 Canvas::onPaint
移除 `event.Skip()` 避免传播：
```cpp
void Canvas::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    render(false);
    if (m_eventCoordinator) {
        m_eventCoordinator->handlePaintEvent(event);
    }
    // event.Skip();  // 移除这行
}
```

### 3. 添加渲染状态管理
防止重复渲染：
```cpp
class Canvas {
    bool m_isRendering = false;
    
    void render(bool fastMode) {
        if (m_isRendering) return;  // 防止重入
        m_isRendering = true;
        // ... 渲染代码 ...
        m_isRendering = false;
    }
};
```

### 4. 优化鼠标悬停检测
添加节流机制：
```cpp
class Canvas {
    wxPoint m_lastHoverPos;
    int m_hoverUpdateCounter = 0;
    
    void onMouseEvent(wxMouseEvent& event) {
        if (event.GetEventType() == wxEVT_MOTION && m_occViewer) {
            wxPoint pos = event.GetPosition();
            // 只在移动超过一定距离或每N次更新一次
            if ((pos - m_lastHoverPos).GetVectorLength() > 5 || 
                ++m_hoverUpdateCounter % 3 == 0) {
                m_occViewer->updateHoverSilhouetteAt(pos);
                m_lastHoverPos = pos;
            }
        }
    }
};
```

### 5. OpenGL 上下文优化
缓存上下文状态，减少不必要的切换：
```cpp
class RenderingEngine {
    bool m_contextCurrent = false;
    
    bool ensureContext() {
        if (!m_contextCurrent) {
            m_contextCurrent = m_canvas->SetCurrent(*m_glContext);
        }
        return m_contextCurrent;
    }
};
```