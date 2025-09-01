# Docking 性能问题深度分析

## 关键信息
- **使用 docking 前**：Canvas 性能正常，鼠标响应流畅
- **使用 docking 后**：鼠标操作卡顿，但功能正常

## 可能的根本原因

### 1. Canvas Reparenting 的副作用
当 Canvas 被 reparent 到 DockWidget 时，可能发生了：
- OpenGL 上下文状态改变
- 事件处理链改变
- 窗口层级关系改变

### 2. 事件传播路径变长
原始结构：
```
Frame -> Canvas
```

Docking 结构：
```
Frame -> DockManager -> DockContainerWidget -> DockArea -> DockWidget -> Canvas
```

### 3. 额外的布局计算
DockWidget 可能在每次鼠标事件时触发布局重算。

### 4. 双重事件处理
可能同时存在多个事件处理器处理同一事件。

## 调查步骤

### 1. 检查事件传播
- 查看 DockWidget 是否拦截或延迟事件
- 检查是否有事件过滤器

### 2. 检查渲染触发
- DockWidget 是否在鼠标移动时触发额外的重绘
- 是否有不必要的 Refresh 调用

### 3. OpenGL 上下文
- 检查 Canvas 的 GL 上下文是否被正确保持
- 是否有上下文切换开销