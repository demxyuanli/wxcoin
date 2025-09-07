# 拖拽停靠系统工作流程

## 概述

拖拽停靠系统允许用户通过拖动窗口标题栏来重新排列界面布局。方向指示器（DockOverlay）提供视觉反馈，指示窗口可以停靠的位置。

## 核心组件

### 1. DockManager
- 管理整个停靠系统
- 处理拖拽事件的开始、移动和结束
- 协调各组件之间的交互

### 2. DockWidget
- 可停靠的窗口单元
- 包含标题栏和内容区域
- 可以被拖动和重新停靠

### 3. DockArea
- 容纳一个或多个 DockWidget 的区域
- 多个 DockWidget 在同一 DockArea 中以标签页形式显示

### 4. DockOverlay
- 方向指示器组件
- 显示可停靠位置的视觉提示
- 两种模式：
  - `ModeDockAreaOverlay`: 针对特定 DockArea 的停靠
  - `ModeContainerOverlay`: 针对整个容器的停靠

## 拖拽停靠流程

### 第一阶段：拖拽开始

```
用户按下鼠标 → DockWidget::OnMouseLeftDown
                ↓
         开始拖拽检测
                ↓
    DockManager::startDrag(DockWidget*)
                ↓
         设置拖拽状态
```

#### 关键代码逻辑：
```cpp
// DockManager::startDrag
void DockManager::startDrag(DockWidget* widget) {
    m_draggedWidget = widget;
    m_dragState = DragFloating;
    
    // 创建浮动预览窗口
    createFloatingPreview(widget);
}
```

### 第二阶段：拖拽移动

```
鼠标移动 → DockManager::onMouseMove
            ↓
      更新浮动窗口位置
            ↓
      检测鼠标下的目标
            ↓
    显示/隐藏方向指示器
```

#### 检测流程：
1. **查找目标 DockArea**
   ```cpp
   DockArea* targetArea = findDockAreaUnderMouse(mousePos);
   ```

2. **显示相应的 Overlay**
   - 如果在 DockArea 上：显示 DockAreaOverlay（5个方向）
   - 如果在容器空白区：显示 ContainerOverlay（5个方向）

3. **更新指示器高亮**
   ```cpp
   // DockOverlay::onMouseMove
   DockWidgetArea hoveredArea = InvalidDockWidgetArea;
   for (auto& dropArea : m_dropAreas) {
       if (dropArea->rect().Contains(localPos)) {
           hoveredArea = dropArea->area();
           break;
       }
   }
   ```

### 第三阶段：视觉反馈

```
鼠标悬停在指示器上 → 高亮该指示器
                    ↓
              显示预览区域
                    ↓
            用户看到停靠效果
```

#### 视觉反馈类型：
1. **指示器高亮**：从灰色变为蓝色
2. **预览区域**：半透明蓝色矩形显示停靠后的位置

### 第四阶段：执行停靠

```
用户释放鼠标 → DockManager::onMouseLeftUp
                ↓
         获取当前停靠区域
                ↓
         执行停靠操作
                ↓
         更新布局
```

#### 停靠执行逻辑：
```cpp
void DockManager::performDocking(DockWidget* widget, 
                                DockArea* targetArea, 
                                DockWidgetArea area) {
    switch (area) {
    case LeftDockWidgetArea:
        // 在目标左侧创建新的 DockArea
        insertDockWidget(widget, targetArea, DockLeft);
        break;
    case CenterDockWidgetArea:
        // 添加为标签页
        targetArea->addDockWidget(widget);
        break;
    // ... 其他方向
    }
}
```

## 方向指示器详解

### 1. DockAreaOverlay（目标区域模式）
```
        [↑]
         |
    [←]--[□]--[→]
         |
        [↓]
```
- **上/下/左/右**：在目标 DockArea 的相应方向创建新区域
- **中心**：作为标签页添加到目标 DockArea

### 2. ContainerOverlay（容器模式）
```
         [↑]
         
[←]      [□]      [→]

         [↓]
```
- **上/下/左/右**：在容器的相应边缘创建新区域
- **中心**：替换或合并到中心区域

## 关键决策点

### 1. 何时显示哪种 Overlay？
```cpp
if (mouseOverDockArea && !mouseOverTitleBar) {
    showDockAreaOverlay(targetArea);
} else if (mouseOverContainer) {
    showContainerOverlay();
}
```

### 2. 如何处理不同的停靠区域？
- **边缘停靠**（上/下/左/右）：使用 wxSplitterWindow 分割
- **中心停靠**：添加为标签页或替换内容

### 3. 布局更新策略
```cpp
// 使用配置的尺寸
int size = getConfiguredAreaSize(area);
splitter->SetSashPosition(size);
```

## 事件流程图

```
MouseDown → StartDrag → MouseMove → UpdateOverlay → MouseUp → PerformDock
    |           |           |             |              |           |
    v           v           v             v              v           v
检测拖拽    创建预览    移动预览    更新指示器    检测目标    执行停靠
                                         |
                                         v
                                   高亮/预览
```

## 性能优化

1. **缓存机制**：缓存 DropArea 的位置，避免重复计算
2. **延迟更新**：使用定时器批量处理布局更新
3. **智能重绘**：只在必要时刷新 Overlay

## 用户体验考虑

1. **即时反馈**：鼠标移动时立即显示指示器
2. **清晰预览**：半透明预览显示最终效果
3. **平滑动画**：可选的动画效果使过渡更自然
4. **容错设计**：无效操作不会破坏现有布局