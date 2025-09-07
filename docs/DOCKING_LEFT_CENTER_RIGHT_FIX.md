# 左中右布局修复说明

## 问题分析

上下布局已经正确，但左中右三个 dock 面板没有正确添加和分割。问题在于：

1. `handleMiddleLayerArea` 方法没有正确处理中间层的创建和管理
2. 当添加第二个和第三个中间层区域时，分割逻辑不正确
3. `findOrCreateMiddleLayer` 方法的逻辑过于简单

## 修复内容

### 1. 改进了 `handleMiddleLayerArea` 方法

- 添加了更详细的日志
- 修复了创建新 splitter 时的父子关系问题
- 正确处理了 existingArea 的 reparent 操作

### 2. 添加了新方法 `addDockAreaToMiddleSplitter`

专门处理向中间层 splitter 添加 dock area 的逻辑：

- 处理空 splitter 的情况
- 处理只有一个窗口的情况
- 处理已有两个窗口需要创建三向分割的复杂情况

### 3. 改进了 `findOrCreateMiddleLayer` 方法

- 添加详细的调试日志
- 更智能地识别中间层：
  - 检查 splitter 的分割模式
  - 如果根是水平分割，查找垂直分割的子 splitter 作为中间层
  - 如果根是垂直分割，根本身就是中间层

## 关键逻辑

### 三向分割的处理

当中间层已经有两个窗口时，添加第三个需要：

1. 创建一个新的子 splitter
2. 将现有的两个窗口移到子 splitter
3. 根据新区域的类型（左/中/右）重新组织布局

```cpp
if (area == LeftDockWidgetArea) {
    // 新区域在左，其他内容移到右边的子splitter
    subSplitter->SplitVertically(window1, window2);
    middleSplitter->SplitVertically(dockArea, subSplitter);
} else if (area == RightDockWidgetArea) {
    // 新区域在右，其他内容移到左边的子splitter
    subSplitter->SplitVertically(window1, window2);
    middleSplitter->SplitVertically(subSplitter, dockArea);
} else { // CenterDockWidgetArea
    // 新区域在中间，需要更复杂的处理
    subSplitter->SplitVertically(dockArea, window2);
    middleSplitter->ReplaceWindow(window2, subSplitter);
}
```

## 预期效果

修复后应该能正确创建左中右布局：

```
+-------------------------------------+
|          Menu Bar (Top)             |
+--------+------------------+---------+
| Toolbox|    Main View     |  Props  |
| (Left) |    (Center)      | (Right) |
|        |                  |         |
+--------+------------------+---------+
|        Output Panel (Bottom)         |
+-------------------------------------+
```

## 调试提示

通过查看调试日志可以了解布局创建过程：

- `findOrCreateMiddleLayer` - 显示如何查找或创建中间层
- `handleMiddleLayerArea` - 显示如何处理左中右区域
- `addDockAreaToMiddleSplitter` - 显示具体的分割操作

## 注意事项

1. wxSplitter 只能分成两部分，所以需要嵌套 splitter 来实现三向分割
2. Reparent 操作的顺序很重要，必须在正确的时机进行
3. SetSashPosition 可能需要在窗口显示后调整才能生效