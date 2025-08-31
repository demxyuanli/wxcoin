# 停靠逻辑更新总结

## 实施的更改

### 1. 区分中心停靠和方向停靠

修改了 `DockContainerWidget::addDockWidget` 方法：

```cpp
// 原逻辑：只要有 targetDockArea 就添加为标签
if (targetDockArea) {
    targetDockArea->addDockWidget(dockWidget);
    return targetDockArea;
}

// 新逻辑：只有中心停靠才添加为标签
if (targetDockArea && area == CenterDockWidgetArea) {
    targetDockArea->addDockWidget(dockWidget);
    return targetDockArea;
}
```

### 2. 实现相对停靠

当有 `targetDockArea` 且不是中心停靠时，使用新的 `addDockAreaRelativeTo` 方法：

```cpp
if (targetDockArea && area != CenterDockWidgetArea) {
    addDockAreaRelativeTo(newDockArea, area, targetDockArea);
} else {
    addDockArea(newDockArea, area);
}
```

### 3. 新增 addDockAreaRelativeTo 方法

该方法实现了相对于特定 DockArea 的分割停靠：

- 找到目标区域的父分割器
- 创建新的子分割器
- 根据停靠方向配置分割方式
- 替换原有布局结构

## 停靠行为总结

### DockAreaOverlay（区域指示器）

当拖动到特定 DockArea 上时：

1. **中心指示器**（CenterDockWidgetArea）
   - 行为：合并为标签页
   - 实现：`targetDockArea->addDockWidget(dockWidget)`
   - 效果：多个窗口在同一区域以标签形式展示

2. **四个方向指示器**
   - 行为：相对于目标区域创建分割
   - 实现：`addDockAreaRelativeTo(newArea, direction, targetArea)`
   - 效果：
     - 上：新区域在目标区域上方
     - 下：新区域在目标区域下方
     - 左：新区域在目标区域左侧
     - 右：新区域在目标区域右侧

### ContainerOverlay（容器指示器）

当拖动到容器空白区域时：
- 所有指示器都在容器级别创建新区域
- 使用通用的 `addDockArea` 方法

## 技术细节

### 分割器层次结构

相对停靠时的结构变化：

```
停靠前:
ParentSplitter
├── TargetArea
└── OtherWindow

停靠后（以左停靠为例）:
ParentSplitter
├── SubSplitter
│   ├── NewArea (左)
│   └── TargetArea (右)
└── OtherWindow
```

### 尺寸配置

- 使用 `getConfiguredAreaSize(area)` 获取配置的区域大小
- 支持像素和百分比两种模式
- 根据停靠方向正确设置分割条位置

## 测试场景

1. **标签合并测试**
   - 拖动到区域中心指示器
   - 验证窗口作为标签添加

2. **方向分割测试**
   - 拖动到区域四个方向指示器
   - 验证正确创建分割布局

3. **容器停靠测试**
   - 拖动到容器空白区域
   - 验证在容器级别创建新区域

4. **复杂布局测试**
   - 在已有多个分割的布局中测试
   - 验证布局结构保持正确