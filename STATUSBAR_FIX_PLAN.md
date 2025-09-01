# 状态栏修复计划

## 问题分析

1. **崩溃原因**：
   - `BorderlessFrameLogic::addStatusBar()` 尝试将 `m_statusBar` 添加到 `m_mainSizer`
   - 但是 `m_mainSizer` 已经被 `FlatFrameDocking::InitializeDockingLayout` 中的 `SetSizer(nullptr)` 破坏了
   - 导致访问无效指针

2. **继承链**：
   ```
   BorderlessFrameLogic
        ↑
   FlatUIFrame
        ↑
   FlatFrame
        ↑
   FlatFrameDocking
   ```

3. **状态栏创建流程**：
   - `BorderlessFrameLogic` 构造函数创建 `m_statusBar`
   - `FlatFrameInit::createPanels()` 调用 `addStatusBar()`
   - `FlatFrameDocking::InitializeDockingLayout()` 重新设置布局

## 解决方案

### 方案 A：保护 m_mainSizer（已实施）
- 不使用 `SetSizer(nullptr)`，而是清空现有 sizer
- 修改 `addStatusBar()` 添加空指针检查

### 方案 B：延迟状态栏创建
- 在 `FlatFrameDocking` 中自己管理状态栏
- 不依赖基类的 `addStatusBar()`

### 方案 C：重构初始化顺序
- 确保基类完全初始化后再修改布局
- 保护关键成员变量

## 当前实施

采用了方案 A + 部分方案 B：
1. 移除了 `SetSizer(nullptr)`，改为清空现有 sizer
2. 修改了 `addStatusBar()` 添加空指针检查
3. 在 `FlatFrameDocking` 中直接管理状态栏，而不是调用 `addStatusBar()`