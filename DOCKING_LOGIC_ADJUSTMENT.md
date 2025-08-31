# 停靠逻辑调整方案

## 当前行为

目前的停靠逻辑：
- 拖动到任何方向指示器都会创建新的分割区域
- 中心指示器的行为不明确

## 期望行为

### 1. DockAreaOverlay（区域方向指示器）
当拖动到特定 DockArea 上时显示的指示器：

- **中心指示器**：合并为标签页
  - 将拖动的 DockWidget 添加到目标 DockArea 中
  - 多个 DockWidget 以标签形式展示
  
- **四个方向指示器**：创建分割区域
  - 上：在目标区域上方创建新区域
  - 下：在目标区域下方创建新区域
  - 左：在目标区域左侧创建新区域
  - 右：在目标区域右侧创建新区域

### 2. ContainerOverlay（容器方向指示器）
当拖动到容器空白区域时显示的指示器：

- **所有指示器**：在容器的相应位置创建新区域
  - 保持现有行为

## 需要修改的代码

### 1. DockManager::addDockWidget
需要根据 area 参数和 targetDockArea 参数来区分行为：

```cpp
DockArea* DockManager::addDockWidget(DockWidgetArea area, 
                                    DockWidget* dockWidget, 
                                    DockArea* targetDockArea) {
    // 如果有目标区域且是中心停靠，添加为标签
    if (targetDockArea && area == CenterDockWidgetArea) {
        return addDockWidgetTabToArea(dockWidget, targetDockArea);
    }
    
    // 其他情况创建新的分割区域
    // ...现有逻辑
}
```

### 2. DockContainerWidget::addDockWidget
需要确保正确处理中心停靠：

```cpp
DockArea* DockContainerWidget::addDockWidget(DockWidgetArea area, 
                                            DockWidget* dockWidget, 
                                            DockArea* targetDockArea, 
                                            int index) {
    // 如果有目标区域且是中心停靠，直接添加到该区域
    if (targetDockArea && area == CenterDockWidgetArea) {
        targetDockArea->addDockWidget(dockWidget);
        return targetDockArea;
    }
    
    // 四个方向的停靠需要创建新区域和分割
    // ...继续现有的分割逻辑
}
```

## 实现步骤

1. 修改 `DockManager::addDockWidget` 以正确处理中心停靠
2. 确保 `DockContainerWidget::addDockWidget` 区分标签合并和分割创建
3. 验证拖放系统正确传递 targetDockArea 参数
4. 测试各种停靠场景