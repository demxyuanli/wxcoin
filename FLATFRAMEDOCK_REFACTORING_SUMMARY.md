# FlatFrameDock 改造总结

## 实施的更改

### 1. 布局结构调整

从原布局：
```
+------------------+----------+
|  Object Tree     | Canvas   | Properties
|  + Toolbox(Tab)  |          |
+------------------+----------+
|            Output            |
+-----------------------------+
```

改为新布局：
```
+---------------+---------------+
| Object Tree   |               |
| (Left-Top)    |               |
|---------------|    Canvas     |
| Properties    |   (Center)    |
| (Left-Bottom) |               |
+---------------+---------------+
| Message | Performance (Tabs)  |
+-------------------------------+
```

### 2. 代码修改详情

#### A. FlatFrameDocking.h
- 添加 `m_performanceDock` 成员变量
- 重命名 `m_outputDock` 为 `m_messageDock`
- 添加 `CreatePerformanceDockWidget()` 方法声明
- 重命名 `CreateOutputDockWidget()` 为 `CreateMessageDockWidget()`
- 更新菜单 ID 枚举，添加 `ID_VIEW_MESSAGE` 和 `ID_VIEW_PERFORMANCE`

#### B. FlatFrameDocking.cpp
主要修改：

1. **CreateDockingLayout()** - 完全重写布局创建逻辑：
   ```cpp
   // 1. 中心画布
   m_canvasDock = CreateCanvasDockWidget();
   m_dockManager->addDockWidget(CenterDockWidgetArea, m_canvasDock);
   
   // 2. 左上 - Object Tree
   m_objectTreeDock = CreateObjectTreeDockWidget();
   DockArea* leftTopArea = m_dockManager->addDockWidget(LeftDockWidgetArea, m_objectTreeDock);
   
   // 3. 左下 - Properties (相对于 Object Tree 下方分割)
   m_propertyDock = CreatePropertyDockWidget();
   m_dockManager->addDockWidget(BottomDockWidgetArea, m_propertyDock, leftTopArea);
   
   // 4. 底部 - Message
   m_messageDock = CreateMessageDockWidget();
   DockArea* bottomArea = m_dockManager->addDockWidget(BottomDockWidgetArea, m_messageDock);
   
   // 5. 底部标签 - Performance
   m_performanceDock = CreatePerformanceDockWidget();
   m_dockManager->addDockWidget(CenterDockWidgetArea, m_performanceDock, bottomArea);
   ```

2. **新增 CreatePerformanceDockWidget()**：
   - 创建性能监控面板
   - 显示 FPS、内存、CPU、渲染时间、三角形数量

3. **ConfigureDockManager()**：
   - 添加默认布局尺寸配置
   - 左侧宽度：300px
   - 底部高度：150px

4. **菜单更新**：
   - 添加 Message 和 Performance 视图菜单项
   - 保留向后兼容性（OUTPUT 映射到 MESSAGE）

5. **事件处理更新**：
   - 更新 `OnViewShowHidePanel()` 处理新的面板
   - 更新 `OnUpdateUI()` 处理新的菜单项

### 3. 关键技术点

#### 相对停靠
使用 `addDockWidget` 的第三个参数实现相对停靠：
```cpp
// Properties 相对于 Object Tree 下方
m_dockManager->addDockWidget(BottomDockWidgetArea, m_propertyDock, leftTopArea);
```

#### 标签合并
使用 `CenterDockWidgetArea` 将窗口添加为标签：
```cpp
// Performance 作为 Message 的标签
m_dockManager->addDockWidget(CenterDockWidgetArea, m_performanceDock, bottomArea);
```

### 4. 移除的功能

- **Toolbox**：不再需要，已从布局中移除
- 保留了相关代码以保持向后兼容性

### 5. 优势

1. **更清晰的三区布局**：
   - 左侧：工具和属性
   - 中心：主要工作区
   - 底部：信息输出

2. **更好的空间利用**：
   - 左侧横向分割（水平分割线）适合查看对象树和编辑属性
   - 底部标签页节省空间

3. **符合专业应用标准**：
   - 类似于 Visual Studio、Unity、Blender 等专业软件的布局

### 6. 后续建议

1. **性能监控实现**：
   - 连接实际的性能数据源
   - 定时更新性能指标

2. **布局持久化**：
   - 保存用户自定义的面板大小
   - 记住面板的显示/隐藏状态

3. **快捷键优化**：
   - 确保所有面板都有合适的快捷键
   - 添加布局切换快捷键

4. **主题支持**：
   - 为新面板添加暗色主题支持
   - 统一面板的视觉风格