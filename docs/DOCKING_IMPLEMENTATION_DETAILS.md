# 拖拽停靠实现细节

## 代码层面的实现逻辑

### 1. 拖拽检测和启动

#### DockWidget 标题栏事件处理
```cpp
// DockWidget 或其标题栏
void onMouseLeftDown(wxMouseEvent& event) {
    if (isInTitleBar(event.GetPosition())) {
        m_dragStartPos = event.GetPosition();
        m_potentialDrag = true;
        CaptureMouse();
    }
}

void onMouseMove(wxMouseEvent& event) {
    if (m_potentialDrag && !m_isDragging) {
        // 检测是否移动足够距离以开始拖拽
        wxPoint delta = event.GetPosition() - m_dragStartPos;
        if (abs(delta.x) > 3 || abs(delta.y) > 3) {
            m_isDragging = true;
            dockManager()->startDrag(this);
        }
    }
}
```

### 2. DockManager 拖拽状态管理

```cpp
class DockManager {
private:
    // 拖拽状态
    enum DragState {
        DragInactive,
        DragFloating,
        DragOverlay
    };
    
    DragState m_dragState;
    DockWidget* m_draggedWidget;
    wxPoint m_dragOffset;
    
    // Overlay 管理
    DockOverlay* m_containerOverlay;
    DockOverlay* m_dockAreaOverlay;
    DockArea* m_targetArea;
};
```

### 3. Overlay 显示逻辑

```cpp
void DockManager::updateDragOverlay(const wxPoint& globalPos) {
    // 1. 查找鼠标下的目标
    DockArea* areaUnderMouse = nullptr;
    DockContainerWidget* container = nullptr;
    
    // 遍历所有容器查找目标
    for (auto* cont : m_containers) {
        wxPoint localPos = cont->ScreenToClient(globalPos);
        if (cont->GetRect().Contains(localPos)) {
            container = cont;
            // 查找具体的 DockArea
            areaUnderMouse = container->dockAreaAt(localPos);
            break;
        }
    }
    
    // 2. 决定显示哪种 Overlay
    if (areaUnderMouse && !areaUnderMouse->titleBarRect().Contains(localPos)) {
        // 显示 DockArea 特定的 overlay
        showDockAreaOverlay(areaUnderMouse);
        hideContainerOverlay();
    } else if (container) {
        // 显示容器级别的 overlay
        showContainerOverlay(container);
        hideDockAreaOverlay();
    } else {
        // 不在任何有效目标上
        hideAllOverlays();
    }
}
```

### 4. 停靠执行实现

```cpp
void DockManager::dropWidget(const wxPoint& globalPos) {
    if (!m_draggedWidget) return;
    
    // 1. 确定停靠位置
    DockWidgetArea dropArea = InvalidDockWidgetArea;
    DockArea* targetArea = nullptr;
    
    if (m_dockAreaOverlay && m_dockAreaOverlay->IsShown()) {
        dropArea = m_dockAreaOverlay->visibleDropAreaUnderMouse(
            m_dockAreaOverlay->ScreenToClient(globalPos));
        targetArea = m_targetArea;
    } else if (m_containerOverlay && m_containerOverlay->IsShown()) {
        dropArea = m_containerOverlay->visibleDropAreaUnderMouse(
            m_containerOverlay->ScreenToClient(globalPos));
    }
    
    // 2. 执行停靠
    if (dropArea != InvalidDockWidgetArea) {
        performDocking(m_draggedWidget, targetArea, dropArea);
    } else {
        // 创建浮动窗口
        createFloatingWidget(m_draggedWidget, globalPos);
    }
    
    // 3. 清理状态
    cleanupDrag();
}
```

### 5. 布局更新机制

```cpp
void DockContainerWidget::insertDockWidget(DockWidget* widget, 
                                          DockArea* relativeTo,
                                          DockWidgetArea area) {
    // 1. 从原位置移除
    if (widget->dockArea()) {
        widget->dockArea()->removeDockWidget(widget);
    }
    
    // 2. 创建新的 DockArea（如果需要）
    DockArea* newArea = new DockArea(this);
    newArea->addDockWidget(widget);
    
    // 3. 更新 splitter 布局
    switch (area) {
    case LeftDockWidgetArea:
        insertAreaLeft(newArea, relativeTo);
        break;
    case RightDockWidgetArea:
        insertAreaRight(newArea, relativeTo);
        break;
    case TopDockWidgetArea:
        insertAreaTop(newArea, relativeTo);
        break;
    case BottomDockWidgetArea:
        insertAreaBottom(newArea, relativeTo);
        break;
    case CenterDockWidgetArea:
        // 合并到现有区域
        relativeTo->addDockWidget(widget);
        delete newArea;
        break;
    }
}
```

### 6. Splitter 操作细节

```cpp
void DockContainerWidget::insertAreaLeft(DockArea* newArea, 
                                        DockArea* relativeTo) {
    // 1. 找到包含 relativeTo 的 splitter
    DockSplitter* parentSplitter = findParentSplitter(relativeTo);
    
    if (!parentSplitter) {
        // 创建根 splitter
        m_rootSplitter = new DockSplitter(this);
        m_rootSplitter->SplitVertically(newArea, relativeTo);
        m_rootSplitter->SetSashPosition(getConfiguredAreaSize(LeftDockWidgetArea));
    } else {
        // 复杂情况：需要重组现有布局
        if (parentSplitter->GetOrientation() == wxVERTICAL) {
            // 相同方向，直接插入
            insertIntoVerticalSplitter(parentSplitter, newArea, relativeTo, true);
        } else {
            // 不同方向，需要创建子 splitter
            createSubSplitter(parentSplitter, newArea, relativeTo, LeftDockWidgetArea);
        }
    }
}
```

## 关键数据结构

### DropArea 信息
```cpp
class DockOverlayDropArea {
    DockWidgetArea m_area;      // 停靠方向
    wxRect m_rect;              // 指示器位置
    bool m_highlighted;         // 是否高亮
    wxRect m_previewRect;       // 预览区域
};
```

### 拖拽状态信息
```cpp
struct DragOperation {
    DockWidget* widget;         // 被拖拽的窗口
    wxPoint startPos;           // 开始位置
    wxPoint offset;             // 鼠标偏移
    DockArea* sourceArea;       // 原始区域
    bool isFloating;            // 是否浮动
    wxBitmap preview;           // 预览位图
};
```

## 性能和用户体验优化

### 1. 减少重绘
```cpp
// 使用脏区域标记
void DockOverlay::updateHighlight(DockWidgetArea area) {
    if (area != m_lastHighlightedArea) {
        // 只重绘改变的区域
        wxRect oldRect = areaRect(m_lastHighlightedArea);
        wxRect newRect = areaRect(area);
        
        RefreshRect(oldRect);
        RefreshRect(newRect);
        
        m_lastHighlightedArea = area;
    }
}
```

### 2. 平滑动画
```cpp
// 可选的动画效果
void animateDocking(DockArea* area, const wxRect& fromRect, const wxRect& toRect) {
    const int steps = 10;
    const int duration = 200; // ms
    
    for (int i = 0; i <= steps; ++i) {
        float t = float(i) / steps;
        wxRect currentRect = interpolateRect(fromRect, toRect, t);
        area->SetSize(currentRect);
        area->Update();
        wxMilliSleep(duration / steps);
    }
}
```

## 边界情况处理

1. **拖拽到自己**：检测并忽略
2. **拖拽到子窗口**：防止循环引用
3. **最小尺寸限制**：确保布局合理
4. **取消操作**：ESC 键取消拖拽
5. **多显示器**：正确处理跨屏幕拖拽