# Dock框架功能说明文档

## 概述

本Dock框架是一个基于wxWidgets的现代化停靠窗口管理系统，提供了灵活的面板布局、拖拽操作和多标签支持。框架设计遵循Visual Studio 2022的界面风格，提供直观的用户体验。

## 核心功能

### 1. 面板管理

#### 1.1 停靠区域
框架支持以下五个停靠区域：
- **Left（左侧）**：支持停靠，可拖拽，显示系统按钮
- **Right（右侧）**：**已禁用停靠**，面板不可拖拽，不显示系统按钮
- **Top（顶部）**：支持停靠，可拖拽，显示系统按钮
- **Bottom（底部）**：支持停靠，可拖拽，显示系统按钮，支持多标签
- **Center（中心）**：**已禁用停靠**，面板不可拖拽，不显示系统按钮
- **Floating（浮动）**：独立浮动窗口

#### 1.2 面板状态
- **Normal（正常）**：正常停靠状态
- **Maximized（最大化）**：最大化显示
- **Minimized（最小化）**：最小化隐藏
- **Floating（浮动）**：独立窗口
- **Hidden（隐藏）**：完全隐藏

### 2. 多标签系统

#### 2.1 标签显示
- **标题栏标签**：当面板包含多个内容时，标签显示在标题栏中
- **FlatBar风格**：采用FlatBar的视觉样式，活动标签有蓝色顶部边框
- **标签切换**：点击标签可切换显示的内容
- **标签居中**：标签在标题栏中居中显示

#### 2.2 标签功能
- **自动创建**：Message和Performance面板会自动创建多标签面板
- **内容切换**：支持在不同内容之间无缝切换
- **关闭按钮**：每个标签可配置关闭按钮（根据TabCloseMode设置）

### 3. 拖拽操作

#### 3.1 拖拽启动
- **拖拽阈值**：鼠标移动超过8像素才启动拖拽操作
- **影子窗口**：拖拽时显示半透明的影子窗口跟随鼠标
- **智能识别**：区分点击切换和拖拽操作，避免误操作

#### 3.2 停靠指示器
- **中心指示器**：始终显示在屏幕中央，提供视觉参考
- **方向指示器**：显示可停靠的方向（Left, Top, Bottom）
- **禁用区域**：Center和Right区域的指示器不响应拖拽操作
- **实时反馈**：鼠标悬停时高亮显示停靠预览

#### 3.3 停靠限制
- **Center区域**：完全禁止停靠，但中心指示器保持可见
- **Right区域**：完全禁止停靠，不显示停靠预览
- **其他区域**：正常支持停靠操作

### 4. 系统按钮

#### 4.1 显示规则
- **启用区域**：Left, Top, Bottom区域的面板显示系统按钮
- **禁用区域**：Center, Right区域的面板不显示系统按钮
- **位置**：系统按钮位于标题栏右侧

#### 4.2 按钮类型
- **最小化**：将面板最小化
- **关闭**：关闭面板
- **浮动**：将面板转为浮动窗口
- **自定义**：支持添加自定义按钮

## 技术架构

### 1. 核心组件

#### 1.1 ModernDockManager
- **功能**：整个停靠系统的核心管理器
- **职责**：面板管理、布局控制、拖拽协调
- **接口**：实现IDockManager接口

#### 1.2 ModernDockPanel
- **功能**：单个停靠面板的实现
- **特性**：支持多标签、自定义渲染、事件处理
- **控制**：拖拽启用/禁用、系统按钮显示/隐藏

#### 1.3 DockGuides
- **功能**：停靠指示器的视觉反馈
- **组件**：中心指示器、边缘指示器
- **控制**：方向启用/禁用、可见性控制

#### 1.4 DragDropController
- **功能**：拖拽操作的控制器
- **职责**：拖拽状态管理、碰撞检测、停靠验证

### 2. 设计模式

#### 2.1 观察者模式
- 拖拽事件的回调机制
- 面板状态变化的通知

#### 2.2 策略模式
- 不同的布局策略（IDE、CAD、Hybrid等）
- 可配置的标签关闭行为

#### 2.3 工厂模式
- 面板的创建和初始化
- 布局组件的构建

## 使用指南

### 1. 基本操作

#### 1.1 添加面板
```cpp
// 添加单个面板
modernDockManager->AddPanel(content, "Panel Title", DockArea::Left);

// 添加多标签面板（Message/Performance会自动合并）
modernDockManager->AddPanel(messagePanel, "Message", DockArea::Bottom);
modernDockManager->AddPanel(performancePanel, "Performance", DockArea::Bottom);
```

#### 1.2 控制停靠行为
```cpp
// 禁用特定面板的拖拽
modernDockManager->SetPanelDockingEnabled(panel, false);

// 禁用整个区域的停靠
modernDockManager->SetAreaDockingEnabled(DockArea::Center, false);

// 控制系统按钮显示
modernDockManager->SetPanelSystemButtonsVisible(panel, false);
```

### 2. 高级配置

#### 2.1 标签设置
```cpp
// 设置标签关闭模式
panel->SetTabCloseMode(TabCloseMode::ShowOnHover);

// 设置标签样式
panel->SetTabStyle(TabStyle::DEFAULT);
panel->SetTabFont(customFont);
```

#### 2.2 布局管理
```cpp
// 设置布局策略
modernDockManager->SetLayoutStrategy(LayoutStrategy::IDE);

// 设置布局约束
LayoutConstraints constraints;
constraints.minPanelSize = wxSize(200, 150);
modernDockManager->SetLayoutConstraints(constraints);
```

### 3. 事件处理

#### 3.1 面板事件
- **选择变化**：标签切换时触发
- **关闭事件**：面板关闭时触发
- **拖拽事件**：开始、更新、完成拖拽

#### 3.2 布局事件
- **布局变化**：面板位置改变时触发
- **大小调整**：面板大小变化时触发

## 配置选项

### 1. 视觉配置

#### 1.1 主题颜色
- **BarBackgroundColour**：标题栏背景色
- **BarActiveTextColour**：活动文本颜色
- **BarActiveTabBgColour**：活动标签背景色
- **BarTabBorderTopColour**：标签顶部边框色

#### 1.2 布局参数
- **DEFAULT_TAB_HEIGHT**：标签高度（24px）
- **DRAG_THRESHOLD**：拖拽阈值（8px）
- **DEFAULT_TAB_SPACING**：标签间距（2px）

### 2. 行为配置

#### 2.1 拖拽设置
- **拖拽阈值**：防止误操作的移动距离
- **动画时长**：布局变化的动画时间
- **影子窗口透明度**：拖拽时的视觉反馈

#### 2.2 标签设置
- **关闭模式**：Always, OnHover, Never
- **最小宽度**：标签的最小显示宽度
- **最大宽度**：标签的最大显示宽度

## 最佳实践

### 1. 性能优化
- 使用双缓冲渲染避免闪烁
- 合理设置更新频率
- 延迟加载面板内容

### 2. 用户体验
- 提供清晰的视觉反馈
- 保持操作的一致性
- 合理的默认布局

### 3. 扩展性
- 使用接口隔离原则
- 支持自定义主题
- 提供插件机制

## 故障排除

### 1. 常见问题

#### 1.1 拖拽不工作
- 检查面板的dockingEnabled状态
- 确认区域没有被禁用
- 验证拖拽阈值设置

#### 1.2 标签显示异常
- 检查内容数量（>1才显示标签）
- 确认标题栏渲染方法
- 验证主题颜色配置

#### 1.3 布局错乱
- 检查布局引擎初始化
- 确认面板的父子关系
- 验证区域分配逻辑

### 2. 调试工具
- 布局树转储功能
- 面板状态检查
- 拖拽事件日志

## 版本历史

### v1.0.0 (当前版本)
- ✅ 完整的停靠面板系统
- ✅ 多标签支持（Message/Performance）
- ✅ 智能拖拽操作
- ✅ 区域停靠控制（禁用Center/Right）
- ✅ FlatBar风格的视觉设计
- ✅ 系统按钮管理
- ✅ 停靠指示器系统

### 未来规划
- 面板分组功能
- 自定义布局模板
- 更多主题选项
- 性能优化改进

---

本文档描述了Dock框架的完整功能和使用方法。如有疑问或需要技术支持，请联系开发团队。

