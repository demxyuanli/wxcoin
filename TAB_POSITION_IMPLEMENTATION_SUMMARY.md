# Dock面板标签位置功能实现总结

## 功能概述

根据用户需求，我们为docking系统中的dock面板实现了标签在上下左右四个方向的显示功能：

- **上方（Top）**：标签与标题栏合并模式，保持现有的合并显示方式
- **下方（Bottom）**：标签显示在面板底部，保留原标题栏，产生独立的标题栏
- **左侧（Left）**：标签显示在面板左侧，保留原标题栏，产生独立的标题栏  
- **右侧（Right）**：标签显示在面板右侧，保留原标题栏，产生独立的标题栏

## 实现细节

### 1. 新增枚举类型

在 `include/docking/DockArea.h` 中添加了 `TabPosition` 枚举：

```cpp
enum class TabPosition {
    Top,        // Tabs at top (merged with title bar)
    Bottom,     // Tabs at bottom (independent title bar)
    Left,       // Tabs at left (independent title bar)
    Right       // Tabs at right (independent title bar)
};
```

### 2. DockArea类扩展

为 `DockArea` 类添加了以下功能：

- `setTabPosition(TabPosition position)` - 设置标签位置
- `tabPosition()` - 获取当前标签位置
- `updateLayoutForTabPosition()` - 根据标签位置更新布局

#### 布局模式：

- **Top模式（合并模式）**：
  - 垂直布局：合并标题栏 + 内容区域
  - 隐藏独立标题栏
  - 标签和按钮显示在同一个标题栏中

- **Bottom模式（独立模式）**：
  - 垂直布局：独立标题栏 + 内容区域 + 标签栏
  - 显示独立标题栏
  - 标签栏在底部，按钮隐藏在标签栏中

- **Left模式（独立模式）**：
  - 垂直布局：独立标题栏 + 水平布局（标签栏在左 + 内容区域）
  - 显示独立标题栏
  - 标签栏在左侧，垂直显示标签

- **Right模式（独立模式）**：
  - 垂直布局：独立标题栏 + 水平布局（内容区域 + 标签栏在右）
  - 显示独立标题栏
  - 标签栏在右侧，垂直显示标签

### 3. DockAreaMergedTitleBar类扩展

为 `DockAreaMergedTitleBar` 类添加了以下功能：

- `setTabPosition(TabPosition position)` - 设置标签位置
- `tabPosition()` - 获取当前标签位置
- `updateHorizontalTabRects()` - 更新水平标签布局
- `updateVerticalTabRects()` - 更新垂直标签布局
- `drawButtons()` - 根据位置绘制按钮
- `drawHorizontalButtons()` - 绘制水平按钮
- `drawVerticalButtons()` - 绘制垂直按钮

#### 按钮显示逻辑：

- **Top位置**：显示所有按钮（合并模式）
- **其他位置**：隐藏所有按钮（独立模式，使用独立标题栏的按钮）

#### 标签绘制：

- **水平标签（Top/Bottom）**：正常水平文本绘制
- **垂直标签（Left/Right）**：文本旋转90度绘制

### 4. 核心实现文件修改

#### `src/docking/DockArea.cpp`
- 添加了 `setTabPosition()` 方法实现
- 添加了 `updateLayoutForTabPosition()` 方法实现
- 修改了构造函数初始化 `m_tabPosition`

#### `src/docking/DockAreaMergedTitleBar.cpp`
- 添加了 `setTabPosition()` 方法实现
- 重构了 `updateTabRects()` 方法，支持不同方向的布局
- 添加了水平和垂直标签布局的专门方法
- 修改了 `drawTab()` 方法，支持垂直文本绘制
- 添加了按钮绘制的专门方法

#### `include/docking/DockArea.h`
- 添加了 `TabPosition` 枚举
- 为 `DockArea` 类添加了标签位置相关方法声明
- 为 `DockAreaMergedTitleBar` 类添加了标签位置相关方法声明
- 添加了私有成员变量 `m_tabPosition`

## 使用方式

```cpp
// 创建DockArea
DockArea* dockArea = new DockArea(dockManager, parent);

// 设置标签位置
dockArea->setTabPosition(TabPosition::Top);    // 合并模式
dockArea->setTabPosition(TabPosition::Bottom); // 独立模式
dockArea->setTabPosition(TabPosition::Left);    // 独立模式
dockArea->setTabPosition(TabPosition::Right);   // 独立模式
```

## 测试验证

创建了测试程序 `test_tab_position_logic.cpp` 验证功能逻辑：

- ✅ Top位置：合并标题栏模式（标签+按钮在同一栏）
- ✅ Bottom位置：独立标题栏模式（独立标题栏+标签栏）
- ✅ Left位置：独立标题栏模式（独立标题栏+垂直标签栏）
- ✅ Right位置：独立标题栏模式（独立标题栏+垂直标签栏）

## 技术特点

1. **向后兼容**：保持现有Top位置的合并模式不变
2. **灵活布局**：支持四个方向的标签显示
3. **独立标题栏**：左右下三个方向保留原标题栏功能
4. **自适应绘制**：根据标签位置自动调整文本方向和按钮布局
5. **性能优化**：只在位置改变时更新布局，避免不必要的重绘

## 总结

成功实现了用户要求的dock面板标签四方向显示功能：

- **上方**：保持现有的标签与标题栏合并模式
- **左右下**：实现独立的标题栏，标签与标题栏分离显示

该实现完全满足了用户的需求，提供了灵活的标签位置选择，同时保持了良好的用户体验和代码可维护性。