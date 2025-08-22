# Dock框架快速入门指南

## 1. 环境准备

### 系统要求
- Windows 10/11
- Visual Studio 2019或更高版本
- wxWidgets 3.2.x
- C++17支持

### 项目配置
确保项目包含以下头文件路径：
```cpp
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"
```

## 2. 基础使用

### 2.1 创建停靠管理器
```cpp
class MainFrame : public wxFrame {
private:
    ModernDockManager* m_dockManager;
    
public:
    MainFrame() : wxFrame(nullptr, wxID_ANY, "应用程序") {
        // 创建停靠管理器
        m_dockManager = new ModernDockManager(this);
        
        // 设置为主布局
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_dockManager, 1, wxEXPAND);
        SetSizer(sizer);
    }
};
```

### 2.2 添加第一个面板
```cpp
void MainFrame::CreatePanels() {
    // 创建工具面板
    auto* toolPanel = new wxPanel(m_dockManager);
    auto* toolSizer = new wxBoxSizer(wxVERTICAL);
    toolSizer->Add(new wxButton(toolPanel, wxID_ANY, "工具1"), 0, wxEXPAND | wxALL, 5);
    toolSizer->Add(new wxButton(toolPanel, wxID_ANY, "工具2"), 0, wxEXPAND | wxALL, 5);
    toolPanel->SetSizer(toolSizer);
    
    // 添加到左侧区域
    m_dockManager->AddPanel(toolPanel, "工具箱", DockArea::Left);
}
```

### 2.3 创建多标签面板
```cpp
void MainFrame::CreateOutputPanels() {
    // 创建Message面板
    auto* messagePanel = new wxTextCtrl(m_dockManager, wxID_ANY, 
        "这是Message面板的内容...", 
        wxDefaultPosition, wxDefaultSize, 
        wxTE_MULTILINE | wxTE_READONLY);
    
    // 创建Performance面板
    auto* performancePanel = new wxPanel(m_dockManager);
    auto* perfSizer = new wxBoxSizer(wxVERTICAL);
    perfSizer->Add(new wxStaticText(performancePanel, wxID_ANY, "性能监控"), 0, wxALL, 5);
    perfSizer->Add(new wxGauge(performancePanel, wxID_ANY, 100), 0, wxEXPAND | wxALL, 5);
    performancePanel->SetSizer(perfSizer);
    
    // 添加到底部区域（会自动合并为多标签）
    m_dockManager->AddPanel(messagePanel, "Message", DockArea::Bottom);
    m_dockManager->AddPanel(performancePanel, "Performance", DockArea::Bottom);
}
```

## 3. 常用功能

### 3.1 控制面板拖拽
```cpp
// 禁用特定面板的拖拽
auto* panel = m_dockManager->FindPanel("工具箱");
if (panel) {
    m_dockManager->SetPanelDockingEnabled(panel, false);
}

// 禁用整个区域的拖拽
m_dockManager->SetAreaDockingEnabled(DockArea::Center, false);
```

### 3.2 隐藏系统按钮
```cpp
// 隐藏特定面板的系统按钮
m_dockManager->SetPanelSystemButtonsVisible(panel, false);

// 或者在面板层面控制
panel->SetSystemButtonsVisible(false);
```

### 3.3 设置标签样式
```cpp
auto* panel = m_dockManager->FindPanel("Output");
if (panel) {
    // 设置标签关闭模式
    panel->SetTabCloseMode(TabCloseMode::ShowOnHover);
    
    // 设置标签字体
    wxFont tabFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    panel->SetTabFont(tabFont);
    
    // 设置标签间距
    panel->SetTabSpacing(4);
}
```

## 4. 完整示例

### 4.1 主框架类
```cpp
class MainFrame : public wxFrame {
public:
    MainFrame() : wxFrame(nullptr, wxID_ANY, "Dock框架示例", 
                         wxDefaultPosition, wxSize(1200, 800)) {
        InitializeDocking();
        CreateMenuBar();
        CreatePanels();
        
        // 居中显示
        Center();
    }

private:
    ModernDockManager* m_dockManager;
    
    void InitializeDocking() {
        // 创建停靠管理器
        m_dockManager = new ModernDockManager(this);
        
        // 设置布局策略
        m_dockManager->SetLayoutStrategy(LayoutStrategy::IDE);
        
        // 配置布局约束
        LayoutConstraints constraints;
        constraints.minPanelSize = wxSize(200, 150);
        constraints.allowFloating = true;
        constraints.animateLayout = true;
        m_dockManager->SetLayoutConstraints(constraints);
        
        // 设置为主布局
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_dockManager, 1, wxEXPAND);
        SetSizer(sizer);
    }
    
    void CreateMenuBar() {
        auto* menuBar = new wxMenuBar();
        auto* viewMenu = new wxMenu();
        
        viewMenu->Append(ID_SHOW_TOOLS, "显示工具箱\tCtrl+1");
        viewMenu->Append(ID_SHOW_OUTPUT, "显示输出\tCtrl+2");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_RESET_LAYOUT, "重置布局");
        
        menuBar->Append(viewMenu, "视图");
        SetMenuBar(menuBar);
        
        // 绑定事件
        Bind(wxEVT_MENU, &MainFrame::OnShowTools, this, ID_SHOW_TOOLS);
        Bind(wxEVT_MENU, &MainFrame::OnShowOutput, this, ID_SHOW_OUTPUT);
        Bind(wxEVT_MENU, &MainFrame::OnResetLayout, this, ID_RESET_LAYOUT);
    }
    
    void CreatePanels() {
        CreateToolPanel();
        CreatePropertiesPanel();
        CreateOutputPanels();
        CreateCenterPanel();
    }
    
    void CreateToolPanel() {
        auto* toolPanel = new wxPanel(m_dockManager);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        
        // 添加工具按钮
        const wxString tools[] = {"选择工具", "绘制工具", "编辑工具", "测量工具"};
        for (const auto& tool : tools) {
            auto* btn = new wxButton(toolPanel, wxID_ANY, tool);
            sizer->Add(btn, 0, wxEXPAND | wxALL, 2);
        }
        
        toolPanel->SetSizer(sizer);
        toolPanel->SetBackgroundColour(wxColour(240, 240, 240));
        
        // 添加到左侧
        m_dockManager->AddPanel(toolPanel, "工具箱", DockArea::Left);
    }
    
    void CreatePropertiesPanel() {
        auto* propPanel = new wxPanel(m_dockManager);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        
        // 属性网格
        sizer->Add(new wxStaticText(propPanel, wxID_ANY, "属性"), 0, wxALL, 5);
        
        auto* listCtrl = new wxListCtrl(propPanel, wxID_ANY, 
                                       wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_SINGLE_SEL);
        listCtrl->AppendColumn("属性", wxLIST_FORMAT_LEFT, 100);
        listCtrl->AppendColumn("值", wxLIST_FORMAT_LEFT, 120);
        
        // 添加示例属性
        listCtrl->InsertItem(0, "名称");
        listCtrl->SetItem(0, 1, "对象1");
        listCtrl->InsertItem(1, "颜色");
        listCtrl->SetItem(1, 1, "红色");
        
        sizer->Add(listCtrl, 1, wxEXPAND | wxALL, 5);
        propPanel->SetSizer(sizer);
        
        // 添加到右侧（会被自动禁用拖拽）
        m_dockManager->AddPanel(propPanel, "属性", DockArea::Right);
    }
    
    void CreateOutputPanels() {
        // Message面板
        auto* messageText = new wxTextCtrl(m_dockManager, wxID_ANY,
            "应用程序启动完成\n准备就绪\n",
            wxDefaultPosition, wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY);
        messageText->SetBackgroundColour(wxColour(250, 250, 250));
        
        // Performance面板
        auto* perfPanel = new wxPanel(m_dockManager);
        auto* perfSizer = new wxBoxSizer(wxVERTICAL);
        
        perfSizer->Add(new wxStaticText(perfPanel, wxID_ANY, "CPU使用率:"), 0, wxALL, 5);
        auto* cpuGauge = new wxGauge(perfPanel, wxID_ANY, 100);
        cpuGauge->SetValue(25);
        perfSizer->Add(cpuGauge, 0, wxEXPAND | wxALL, 5);
        
        perfSizer->Add(new wxStaticText(perfPanel, wxID_ANY, "内存使用:"), 0, wxALL, 5);
        auto* memGauge = new wxGauge(perfPanel, wxID_ANY, 100);
        memGauge->SetValue(60);
        perfSizer->Add(memGauge, 0, wxEXPAND | wxALL, 5);
        
        perfPanel->SetSizer(perfSizer);
        
        // 添加到底部（自动合并为多标签）
        m_dockManager->AddPanel(messageText, "Message", DockArea::Bottom);
        m_dockManager->AddPanel(perfPanel, "Performance", DockArea::Bottom);
    }
    
    void CreateCenterPanel() {
        // 主工作区
        auto* centerPanel = new wxPanel(m_dockManager);
        centerPanel->SetBackgroundColour(wxColour(255, 255, 255));
        
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* text = new wxStaticText(centerPanel, wxID_ANY, 
            "主工作区\n\n这里是应用程序的主要内容区域\n该区域禁止停靠其他面板",
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        text->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        
        sizer->AddStretchSpacer();
        sizer->Add(text, 0, wxALIGN_CENTER);
        sizer->AddStretchSpacer();
        centerPanel->SetSizer(sizer);
        
        // 添加到中心（会被自动禁用拖拽）
        m_dockManager->AddPanel(centerPanel, "主工作区", DockArea::Center);
    }
    
    // 事件处理
    void OnShowTools(wxCommandEvent&) {
        auto* panel = m_dockManager->FindPanel("工具箱");
        if (panel) {
            panel->Show(!panel->IsShown());
        }
    }
    
    void OnShowOutput(wxCommandEvent&) {
        auto* panel = m_dockManager->FindPanel("Output");
        if (panel) {
            panel->Show(!panel->IsShown());
        }
    }
    
    void OnResetLayout(wxCommandEvent&) {
        // 重置布局逻辑
        wxMessageBox("重置布局功能", "信息", wxOK | wxICON_INFORMATION);
    }
    
    enum {
        ID_SHOW_TOOLS = 1001,
        ID_SHOW_OUTPUT,
        ID_RESET_LAYOUT
    };
};
```

### 4.2 应用程序类
```cpp
class DockApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit()) return false;
        
        // 创建主框架
        auto* frame = new MainFrame();
        frame->Show(true);
        
        return true;
    }
};

wxIMPLEMENT_APP(DockApp);
```

## 5. 注意事项

### 5.1 默认配置
- **Center和Right区域**默认禁用停靠
- **Message和Performance面板**会自动合并为多标签
- **拖拽阈值**设置为8像素，避免误操作

### 5.2 最佳实践
1. **面板内容**：确保面板内容有合适的最小尺寸
2. **主题适配**：使用CFG_COLOUR()和CFG_FONT()获取主题设置
3. **事件处理**：合理绑定面板相关事件
4. **内存管理**：面板会自动管理内容窗口的生命周期

### 5.3 常见问题
1. **面板不显示**：检查是否正确添加到停靠管理器
2. **拖拽不工作**：确认面板的dockingEnabled状态
3. **标签不出现**：确保面板有多个内容或正确配置

## 6. 下一步

- 阅读[功能说明文档](DockFramework_UserGuide.md)了解详细功能
- 查看[API参考](DockFramework_API_Reference.md)获取完整接口说明
- 参考[技术实现](DockFramework_TechnicalDetails.md)深入了解内部机制

---

通过这个快速入门指南，您可以快速创建一个具有完整停靠功能的应用程序。框架提供了丰富的定制选项，可以根据具体需求进行调整。
