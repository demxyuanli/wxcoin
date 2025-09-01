# Docking 架构澄清

## 正确的层次结构

```
FlatFrame (整个窗口框架)
├── FlatUIBar (顶部功能区) - 由基类管理
├── 主工作区 (Main Work Area) - 这是 DockManager 管理的区域
│   ├── Left Dock Area (Object Tree, Properties)
│   ├── Center Dock Area (Canvas)
│   └── Bottom Dock Area (Message, Performance)
└── StatusBar (底部状态栏) - 由基类管理
```

## 关键理解

1. **DockManager 的职责范围**：
   - DockManager 只管理主工作区，不管理整个客户端区域
   - FlatUIBar 和 StatusBar 仍由基类 FlatFrame 管理

2. **布局组织**：
   - 基类使用某种方式组织整体布局（可能是 sizer）
   - 我们需要将 DockManager 的容器插入到正确的位置
   - 不应该覆盖整个客户端区域

3. **当前实现的问题**：
   - 我们创建的 workAreaPanel 应该被正确地插入到基类的布局系统中
   - 需要确保它位于 FlatUIBar 之后，StatusBar 之前

## 解决方案

当前的实现基本正确，但需要确保：

1. **基类布局的兼容性**：
   - 检查基类是否已经有一个 sizer
   - 如果有，正确地将 workAreaPanel 插入到合适的位置
   - 如果没有，创建一个新的 sizer 并正确组织所有组件

2. **GetMainWorkArea() 方法**：
   - 提供了一个虚方法让基类知道主工作区在哪里
   - 这允许基类在需要时访问主工作区

3. **事件处理**：
   - onSize 事件的覆盖确保布局更新不会干扰 docking 系统
   - 事件仍然会传播到子窗口

## 待验证的问题

1. FlatUIBar 在哪里创建和添加？
2. 基类的整体布局是如何组织的？
3. 是否需要在特定的时机调用 InitializeDockingLayout？

这些问题需要通过实际运行和调试来验证。