// 测试布局配置功能的简单示例
#include <wx/wx.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockLayoutConfig.h"
#include "docking/DockContainerWidget.h"

using namespace ads;

class TestApp : public wxApp {
public:
    virtual bool OnInit() override {
        // 创建主窗口
        wxFrame* frame = new wxFrame(nullptr, wxID_ANY, "Docking Layout Config Test",
                                    wxDefaultPosition, wxSize(1000, 700));
        
        // 创建菜单
        wxMenuBar* menuBar = new wxMenuBar();
        wxMenu* viewMenu = new wxMenu();
        viewMenu->Append(wxID_ANY, "Configure Layout...", "Configure dock panel sizes");
        menuBar->Append(viewMenu, "&View");
        frame->SetMenuBar(menuBar);
        
        // 创建 DockManager
        DockManager* dockManager = new DockManager(frame);
        
        // 设置默认配置（20/80 布局）
        DockLayoutConfig config;
        config.usePercentage = true;
        config.leftAreaPercent = 20;
        config.bottomAreaPercent = 20;
        dockManager->setLayoutConfig(config);
        
        // 创建一些示例 dock widgets
        auto* leftDock = new DockWidget("Left Panel", dockManager);
        leftDock->setWidget(new wxTextCtrl(leftDock, wxID_ANY, "Left Panel Content", 
                                          wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE));
        
        auto* centerDock = new DockWidget("Center Panel", dockManager);
        centerDock->setWidget(new wxTextCtrl(centerDock, wxID_ANY, "Center Panel Content", 
                                            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE));
        
        auto* bottomDock = new DockWidget("Bottom Panel", dockManager);
        bottomDock->setWidget(new wxTextCtrl(bottomDock, wxID_ANY, "Bottom Panel Content", 
                                            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE));
        
        // 添加到 dock manager
        dockManager->addDockWidget(LeftDockWidgetArea, leftDock);
        dockManager->addDockWidget(CenterDockWidgetArea, centerDock);
        dockManager->addDockWidget(BottomDockWidgetArea, bottomDock);
        
        // 绑定菜单事件
        frame->Bind(wxEVT_MENU, [dockManager, frame](wxCommandEvent&) {
            const DockLayoutConfig& currentConfig = dockManager->getLayoutConfig();
            DockLayoutConfig config = currentConfig;
            DockLayoutConfigDialog dlg(frame, config, dockManager);
            
            if (dlg.ShowModal() == wxID_OK) {
                config = dlg.GetConfig();
                dockManager->setLayoutConfig(config);
                
                // 应用配置
                if (wxWindow* containerWidget = dockManager->containerWidget()) {
                    if (DockContainerWidget* container = dynamic_cast<DockContainerWidget*>(containerWidget)) {
                        container->applyLayoutConfig();
                    }
                }
                
                wxMessageBox("Layout configuration applied!", "Success", wxOK | wxICON_INFORMATION);
            }
        });
        
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);