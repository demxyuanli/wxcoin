/**
 * @brief Simplified docking frame implementation
 * 
 * This version avoids conflicts with base class private members
 * by creating its own widgets independently.
 */

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/listbox.h>
#include <wx/propgrid/propgrid.h>
#include <wx/stattext.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/PerspectiveManager.h"

using namespace ads;

class SimpleDockingFrame : public wxFrame {
public:
    SimpleDockingFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(nullptr, wxID_ANY, title, pos, size)
        , m_dockManager(nullptr)
    {
        InitializeDocking();
    }
    
    virtual ~SimpleDockingFrame() {
        // DockManager will be deleted by its parent
    }
    
private:
    DockManager* m_dockManager;
    
    void InitializeDocking() {
        // Create main panel
        wxPanel* mainPanel = new wxPanel(this);
        
        // Create dock manager
        m_dockManager = new DockManager(mainPanel);
        ConfigureDockManager();
        
        // Create docking layout
        CreateDockingLayout();
        
        // Create menus
        CreateMenus();
        
        // Set up sizers
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(m_dockManager->containerWidget(), 1, wxEXPAND);
        mainPanel->SetSizer(mainSizer);
        
        // Status bar
        CreateStatusBar(2);
        SetStatusText("Docking system ready", 0);
    }
    
    void ConfigureDockManager() {
        m_dockManager->setConfigFlag(OpaqueSplitterResize, true);
        m_dockManager->setConfigFlag(DockAreaHasCloseButton, true);
        m_dockManager->setConfigFlag(AllTabsHaveCloseButton, true);
        m_dockManager->setConfigFlag(FocusHighlighting, true);
    }
    
    void CreateDockingLayout() {
        // 创建示例性的dock panels
        
        // 1. 主视图 - 不可关闭的中心面板
        DockWidget* mainDock = new DockWidget("主视图", m_dockManager->containerWidget());
        wxPanel* mainPanel = new wxPanel(mainDock);
        mainPanel->SetBackgroundColour(wxColour(240, 240, 240));
        
        wxStaticText* mainText = new wxStaticText(mainPanel, wxID_ANY, 
            "这是主视图面板\n不可关闭，始终显示在中心区域",
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(mainText, 1, wxALL | wxEXPAND | wxALIGN_CENTER, 20);
        mainPanel->SetSizer(mainSizer);
        
        mainDock->setWidget(mainPanel);
        mainDock->setFeature(DockWidgetClosable, false);  // 不可关闭
        mainDock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU));
        m_dockManager->addDockWidget(CenterDockWidgetArea, mainDock);
        
        // 2. 工具面板 - 左侧
        DockWidget* toolDock = new DockWidget("工具箱", m_dockManager->containerWidget());
        wxListBox* toolList = new wxListBox(toolDock, wxID_ANY);
        toolList->Append("选择工具");
        toolList->Append("移动工具");
        toolList->Append("缩放工具");
        toolList->Append("旋转工具");
        toolList->Append("画笔工具");
        toolList->SetSelection(0);
        
        toolDock->setWidget(toolList);
        toolDock->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_MENU));
        m_dockManager->addDockWidget(LeftDockWidgetArea, toolDock);
        
        // 3. 属性面板 - 右侧
        DockWidget* propDock = new DockWidget("属性", m_dockManager->containerWidget());
        wxPropertyGrid* propGrid = new wxPropertyGrid(propDock);
        propGrid->Append(new wxStringProperty("名称", wxPG_LABEL, "对象1"));
        propGrid->Append(new wxIntProperty("宽度", wxPG_LABEL, 100));
        propGrid->Append(new wxIntProperty("高度", wxPG_LABEL, 100));
        propGrid->Append(new wxColourProperty("颜色", wxPG_LABEL, *wxBLUE));
        
        propDock->setWidget(propGrid);
        propDock->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));
        m_dockManager->addDockWidget(RightDockWidgetArea, propDock);
        
        // 4. 输出面板 - 底部
        DockWidget* outputDock = new DockWidget("输出", m_dockManager->containerWidget());
        wxTextCtrl* output = new wxTextCtrl(outputDock, wxID_ANY,
                                           "欢迎使用简单停靠示例\n"
                                           "这是一个展示基本停靠功能的示例程序\n"
                                           "- 拖动标签页可以移动面板\n"
                                           "- 拖动到边缘可以停靠\n"
                                           "- 拖动到中央可以创建标签组\n",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
        output->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE));
        
        outputDock->setWidget(output);
        outputDock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
        m_dockManager->addDockWidget(BottomDockWidgetArea, outputDock);
    }
    
    void CreateMenus() {
        wxMenuBar* menuBar = new wxMenuBar();
        
        // File menu
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_EXIT, "E&xit");
        menuBar->Append(fileMenu, "&File");
        
        // View menu
        wxMenu* viewMenu = new wxMenu();
        viewMenu->Append(ID_SAVE_LAYOUT, "Save Layout...\tCtrl+S");
        viewMenu->Append(ID_LOAD_LAYOUT, "Load Layout...\tCtrl+O");
        viewMenu->Append(ID_RESET_LAYOUT, "Reset Layout");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_MANAGE_PERSPECTIVES, "Manage Perspectives...");
        menuBar->Append(viewMenu, "&View");
        
        SetMenuBar(menuBar);
        
        // Bind events
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnSaveLayout, this, ID_SAVE_LAYOUT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnLoadLayout, this, ID_LOAD_LAYOUT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnResetLayout, this, ID_RESET_LAYOUT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnManagePerspectives, this, ID_MANAGE_PERSPECTIVES);
    }
    
    void OnSaveLayout(wxCommandEvent&) {
        wxFileDialog dlg(this, "Save Layout", "", "layout.xml",
                        "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() == wxID_OK) {
            wxString state;
            m_dockManager->saveState(state);
            wxFile file(dlg.GetPath(), wxFile::write);
            if (file.IsOpened()) {
                file.Write(state);
                SetStatusText("Layout saved", 0);
            }
        }
    }
    
    void OnLoadLayout(wxCommandEvent&) {
        wxFileDialog dlg(this, "Load Layout", "", "",
                        "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() == wxID_OK) {
            wxFile file(dlg.GetPath(), wxFile::read);
            if (file.IsOpened()) {
                wxString state;
                file.ReadAll(&state);
                if (m_dockManager->restoreState(state)) {
                    SetStatusText("Layout loaded", 0);
                }
            }
        }
    }
    
    void OnResetLayout(wxCommandEvent&) {
        // Simple reset: remove and recreate
        auto widgets = m_dockManager->dockWidgets();
        for (auto* widget : widgets) {
            m_dockManager->removeDockWidget(widget);
        }
        CreateDockingLayout();
        SetStatusText("Layout reset", 0);
    }
    
    void OnManagePerspectives(wxCommandEvent&) {
        ads::PerspectiveDialog dlg(this, m_dockManager->perspectiveManager());
        dlg.ShowModal();
    }
    
    enum {
        ID_SAVE_LAYOUT = wxID_HIGHEST + 1,
        ID_LOAD_LAYOUT,
        ID_RESET_LAYOUT,
        ID_MANAGE_PERSPECTIVES
    };
};

// Simple test app
class SimpleDockingApp : public wxApp {
public:
    virtual bool OnInit() override {
        SimpleDockingFrame* frame = new SimpleDockingFrame(
            "Simple Docking Frame", 
            wxDefaultPosition, 
            wxSize(1200, 800)
        );
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(SimpleDockingApp);
