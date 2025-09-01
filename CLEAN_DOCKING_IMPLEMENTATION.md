# 清理 Docking 实施方案

## 当前架构

```
MainApplication.cpp    --> FlatFrame (旧布局)
MainApplicationDocking.cpp --> FlatFrameDocking : FlatFrame (新 docking)
```

## 推荐方案：保持两个版本独立

### 方案 A：最小化修改（推荐）

保持当前架构，但清理未使用的代码：

#### 1. 修改 FlatFrame.h

```cpp
// 在 FlatFrame.h 中添加条件编译
#ifndef USE_DOCKING_SYSTEM
    wxAuiManager m_auiManager;
    wxSplitterWindow* m_mainSplitter;
    wxSplitterWindow* m_leftSplitter;
#endif
```

#### 2. 确保 FlatFrameDocking 完全独立

在 FlatFrameDocking 构造函数中：
```cpp
FlatFrameDocking::FlatFrameDocking(...) : FlatFrame(...) {
    // 不调用基类的任何布局初始化
    // 直接初始化新的 docking 系统
    InitializeDockingLayout();
}
```

### 方案 B：完全分离（更彻底）

#### 1. 创建共享基类

创建 `FlatFrameCommon.h`：
```cpp
class FlatFrameCommon : public wxFrame {
protected:
    // 只包含两个版本都需要的功能
    FlatUIBar* m_ribbon;
    CommandManager* m_commandManager;
    // ... 其他共同组件
    
    void InitializeCommonUI();
    void CreateMenus();
    void CreateRibbon();
};
```

#### 2. 两个独立实现

```cpp
// 旧版本
class FlatFrame : public FlatFrameCommon {
    wxAuiManager m_auiManager;
    // ... 旧布局相关
};

// 新版本
class FlatFrameDocking : public FlatFrameCommon {
    DockManager* m_dockManager;
    // ... 新 docking 相关
};
```

## 立即可行的步骤

### 1. 在 FlatFrameDocking 中屏蔽基类布局

修改 `FlatFrameDocking::InitializeDockingLayout()`：

```cpp
void FlatFrameDocking::InitializeDockingLayout() {
    // 确保不使用基类的布局组件
    // 创建一个占满整个客户区的面板
    wxPanel* mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // 创建 dock manager
    m_dockManager = new DockManager(mainPanel);
    mainSizer->Add(m_dockManager->containerWidget(), 1, wxEXPAND);
    
    mainPanel->SetSizer(mainSizer);
    
    // 设置为主窗口的唯一子窗口
    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(mainPanel, 1, wxEXPAND);
    SetSizer(frameSizer);
    
    // 配置和创建 docking 布局
    ConfigureDockManager();
    CreateDockingLayout();
    CreateDockingMenus();
}
```

### 2. 验证独立性

在 FlatFrameDocking 中添加检查：

```cpp
void FlatFrameDocking::OnSize(wxSizeEvent& event) {
    // 只处理 docking 系统的大小调整
    if (m_dockManager && m_dockManager->containerWidget()) {
        // 不要调用基类的 OnSize
        event.Skip();
        return;
    }
}
```

### 3. 清理菜单和事件

确保 docking 版本有自己的事件表：

```cpp
wxBEGIN_EVENT_TABLE(FlatFrameDocking, wxFrame)  // 注意：直接继承 wxFrame 的事件
    // 只包含 docking 相关的事件
wxEND_EVENT_TABLE()
```

## 测试清单

- [ ] FlatFrameDocking 不调用基类的布局方法
- [ ] 所有面板都由 DockManager 管理
- [ ] 菜单和工具栏正常工作
- [ ] 没有内存泄漏或双重删除
- [ ] 窗口调整大小正常
- [ ] 保存/加载布局正常

## 结论

推荐采用**方案 A** - 最小化修改：
1. 保持当前架构
2. 确保 FlatFrameDocking 完全独立使用新 docking 系统
3. 可选：在基类中使用条件编译隐藏未使用的组件

这样可以：
- 保持向后兼容性
- 允许用户选择使用哪个版本
- 最小化代码改动
- 降低引入 bug 的风险