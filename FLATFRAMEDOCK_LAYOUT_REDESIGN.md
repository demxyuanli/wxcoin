# FlatFrameDock 布局改造方案

## 当前布局分析

### 现有布局
```
当前实现：
- Center: Canvas (3D View)
- Left: Object Tree (与 Toolbox 合并为标签)
- Right: Properties
- Bottom: Output
```

### 问题
1. Properties 在右侧，需要移到左下
2. Toolbox 与 Object Tree 合并，但位置不对
3. 缺少 Performance 面板
4. 需要实现左侧上下分割布局

## 目标布局

```
+--------------------------------+
|  Object Tree  |                |
|  (Left-Top)   |                |
|---------------|    Canvas      |
|  Properties   |   (Center)     |
|  (Left-Bottom)|                |
|--------------------------------|
| Message | Performance          |
|      (Bottom - Tabs)           |
+--------------------------------+
```

## 改造步骤

### 1. 修改 CreateDockingLayout 方法

```cpp
void FlatFrameDocking::CreateDockingLayout() {
    // 1. 创建主画布 (中心区域)
    m_canvasDock = CreateCanvasDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_canvasDock);
    
    // 2. 创建 Object Tree (左上)
    m_objectTreeDock = CreateObjectTreeDockWidget();
    DockArea* leftTopArea = m_dockManager->addDockWidget(LeftDockWidgetArea, m_objectTreeDock);
    
    // 3. 创建 Properties (左下) - 相对于 Object Tree 下方分割
    m_propertyDock = CreatePropertyDockWidget();
    m_dockManager->addDockWidget(BottomDockWidgetArea, m_propertyDock, leftTopArea);
    
    // 4. 创建 Message 输出 (底部)
    m_messageDock = CreateMessageDockWidget();
    DockArea* bottomArea = m_dockManager->addDockWidget(BottomDockWidgetArea, m_messageDock);
    
    // 5. 创建 Performance 面板 (底部标签)
    m_performanceDock = CreatePerformanceDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_performanceDock, bottomArea);
    
    // 设置初始焦点
    m_canvasDock->setAsCurrentTab();
}
```

### 2. 新增 Performance 面板

```cpp
DockWidget* FlatFrameDocking::CreatePerformanceDockWidget() {
    DockWidget* dock = new DockWidget("Performance", m_dockManager->containerWidget());
    
    // 创建性能监控面板
    wxPanel* perfPanel = new wxPanel(dock);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // 添加性能指标显示
    wxStaticText* fpsLabel = new wxStaticText(perfPanel, wxID_ANY, "FPS: 0");
    wxStaticText* memLabel = new wxStaticText(perfPanel, wxID_ANY, "Memory: 0 MB");
    wxStaticText* cpuLabel = new wxStaticText(perfPanel, wxID_ANY, "CPU: 0%");
    
    sizer->Add(fpsLabel, 0, wxEXPAND | wxALL, 5);
    sizer->Add(memLabel, 0, wxEXPAND | wxALL, 5);
    sizer->Add(cpuLabel, 0, wxEXPAND | wxALL, 5);
    
    perfPanel->SetSizer(sizer);
    dock->setWidget(perfPanel);
    
    // 配置 dock widget
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);
    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
    
    return dock;
}
```

### 3. 重命名 Output 为 Message

```cpp
DockWidget* FlatFrameDocking::CreateMessageDockWidget() {
    DockWidget* dock = new DockWidget("Message", m_dockManager->containerWidget());
    
    // 使用现有的输出控件逻辑
    wxTextCtrl* output = new wxTextCtrl(dock, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    
    dock->setWidget(output);
    
    // 配置
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, false); // 底部面板通常不浮动
    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
    
    m_outputCtrl = output;
    
    return dock;
}
```

### 4. 移除 Toolbox

由于新布局不需要 Toolbox，可以：
- 删除 `CreateToolboxDockWidget` 方法
- 删除 `m_toolboxDock` 成员变量
- 移除相关菜单项

### 5. 配置默认大小

```cpp
void FlatFrameDocking::ConfigureDockManager() {
    // ... 现有配置 ...
    
    // 设置默认区域大小
    DockLayoutConfig config;
    config.leftAreaWidth = 300;      // 左侧面板宽度
    config.bottomAreaHeight = 150;   // 底部面板高度
    config.usePercentage = false;
    
    m_dockManager->setLayoutConfig(config);
}
```

## 实现文件修改

### FlatFrameDocking.h 修改

```cpp
private:
    ads::DockManager* m_dockManager;
    
    // Dock widgets
    ads::DockWidget* m_canvasDock;
    ads::DockWidget* m_objectTreeDock;
    ads::DockWidget* m_propertyDock;
    ads::DockWidget* m_messageDock;      // 原 m_outputDock
    ads::DockWidget* m_performanceDock;  // 新增
    // 删除: ads::DockWidget* m_toolboxDock;
    
    wxTextCtrl* m_outputCtrl;
```

### 菜单更新

更新视图菜单，移除 Toolbox 项，添加 Performance 项：

```cpp
void FlatFrameDocking::CreateDockingMenus() {
    // ... 
    viewMenu->AppendCheckItem(ID_VIEW_OBJECT_TREE, "Object Tree\tCtrl+Alt+O");
    viewMenu->AppendCheckItem(ID_VIEW_PROPERTIES, "Properties\tCtrl+Alt+P");
    viewMenu->AppendCheckItem(ID_VIEW_MESSAGE, "Message\tCtrl+Alt+M");
    viewMenu->AppendCheckItem(ID_VIEW_PERFORMANCE, "Performance\tCtrl+Alt+F");
    // 删除 Toolbox 菜单项
}
```

## 优势

1. **清晰的三区布局**：左侧工具区、中心工作区、底部信息区
2. **左侧垂直分割**：Object Tree 和 Properties 垂直排列，方便查看和编辑
3. **底部标签页**：Message 和 Performance 共享底部空间，节省屏幕空间
4. **符合 CAD 应用习惯**：这种布局在专业 CAD/3D 应用中很常见

## 注意事项

1. 需要确保 `addDockWidget` 的第三个参数（targetDockArea）正确传递
2. 左侧的上下分割需要使用相对停靠（BottomDockWidgetArea 相对于 leftTopArea）
3. 底部的标签合并使用 CenterDockWidgetArea 相对于 bottomArea