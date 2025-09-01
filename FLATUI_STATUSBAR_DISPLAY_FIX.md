# FlatUIStatusBar 显示问题修复

## 问题描述
FlatUIStatusBar 创建了但没有显示在窗口底部。

## 问题原因

1. **状态栏被错误隐藏**：
   - 在 `InitializeDockingLayout` 中，代码检查 `IsKindOf(CLASSINFO(wxStatusBar))`
   - 但 `FlatUIStatusBar` 不是继承自 `wxStatusBar`，所以检查失败
   - 导致状态栏被 `Hide()` 调用隐藏

2. **基类构造函数问题**：
   - `BorderlessFrameLogic` 创建了状态栏但没有添加到 sizer
   - 这导致状态栏虽然存在但不在布局中

## 修复方案

### 1. 修复隐藏逻辑
```cpp
// 修改前
if (child != ribbon && !child->IsKindOf(CLASSINFO(wxStatusBar))) {
    child->Hide();
}

// 修改后
if (child != ribbon && 
    !child->IsKindOf(CLASSINFO(wxStatusBar)) && 
    child != GetFlatUIStatusBar()) {
    child->Hide();
}
```

### 2. 确保状态栏显示
在 `InitializeDockingLayout` 中：
- 检查状态栏是否存在
- 如果不存在则创建
- 添加到 mainSizer
- 调用 `Show()` 确保可见

### 3. 文档化基类行为
在 `BorderlessFrameLogic` 构造函数中添加注释，说明状态栏创建但不添加到 sizer，让派生类控制布局。

## 效果
- FlatUIStatusBar 正确显示在窗口底部
- 保持了自定义状态栏的所有功能
- 进度条等特性可以正常工作
- 状态栏不会被错误隐藏