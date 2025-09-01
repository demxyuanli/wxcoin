# Canvas 初始化问题深入分析

## 问题现象
1. 创建新 Canvas 后，视口不响应鼠标
2. 导入几何体直接卡住
3. 使用 reparent 的 Canvas 也有卡顿问题

## 初始化流程分析

### 1. 构造函数调用顺序
```
FlatFrame::FlatFrame()
  ├── InitializeUI()
  │   └── createPanels()
  │       └── 检查 IsUsingDockingSystem()
  │           └── 如果是 true，提前返回，不创建 ModernDockAdapter
  └── 基类构造完成

FlatFrameDocking::FlatFrameDocking()
  ├── 基类 FlatFrame 已构造完成
  ├── EnsurePanelsCreated()
  │   ├── 创建 Canvas, PropertyPanel, ObjectTreePanel
  │   └── 设置 MouseHandler, NavigationController, OCCViewer
  └── InitializeDockingLayout()
      └── CreateCanvasDockWidget()
          └── 使用或创建 Canvas
```

### 2. 关键问题

#### 问题1：虚函数调用时机
- `IsUsingDockingSystem()` 是虚函数
- 在基类构造函数中调用虚函数，会调用基类版本（返回 false）
- 这意味着 `createPanels()` 可能会创建 ModernDockAdapter

#### 问题2：Canvas 初始化依赖
Canvas 需要以下组件才能正常工作：
- InputManager 需要 MouseHandler
- MouseHandler 需要 Canvas, ObjectTreePanel, PropertyPanel
- NavigationController 需要 Canvas 和 SceneManager
- OCCViewer 需要 SceneManager

#### 问题3：OpenGL 上下文
- Canvas 继承自 wxGLCanvas
- Reparent 操作可能破坏 OpenGL 上下文
- 需要在正确的时机调用 SetCurrent()

## 根本原因
1. **初始化顺序混乱**：面板在多个地方被创建
2. **依赖关系复杂**：Canvas 依赖的组件分散在不同的初始化阶段
3. **OpenGL 特殊性**：wxGLCanvas 的 reparent 需要特殊处理

## 建议的解决方案

### 方案1：延迟初始化
在 docking 版本中，完全跳过基类的面板创建，所有面板都在 docking 系统中创建。

### 方案2：保护性检查
在每个关键点添加检查，确保组件已正确初始化。

### 方案3：重构初始化流程
将面板创建和连接设置分离，使流程更清晰。