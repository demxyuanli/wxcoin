# 面板创建计划

## 问题分析

1. **当前问题**：
   - Canvas 不响应鼠标操作
   - 导入的几何体不显示
   - Performance 面板不是原来的
   - 这是因为 docking 系统创建了新的面板，而不是使用基类的面板

2. **根本原因**：
   - 基类可能在某个地方创建了面板并设置了事件连接
   - Docking 系统创建了新的面板，失去了这些连接

## 解决方案

1. **EnsurePanelsCreated 方法**：
   - 在基类中实现这个方法
   - 创建所有必要的面板（Canvas, PropertyPanel, ObjectTreePanel, MessageOutput）
   - 不将它们添加到任何布局中（让 docking 系统处理布局）
   - 设置所有必要的事件连接

2. **Docking 系统修改**：
   - 使用基类创建的面板，通过 Reparent 将它们移到 dock widgets
   - 保持所有原有的事件连接和功能

3. **Performance 面板**：
   - 需要确定原来的 Performance 面板是什么
   - 可能需要从基类获取或创建一个真正的性能监控面板

## 实现步骤

1. 在 FlatFrame.cpp 中实现 EnsurePanelsCreated
2. 确保所有面板都被正确创建和初始化
3. 修改 docking 系统使用这些面板
4. 测试功能是否正常