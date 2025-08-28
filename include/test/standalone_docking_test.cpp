/**
 * @brief Standalone docking system test
 * 
 * This can be compiled independently without modifying the main CMakeLists.txt
 * 
 * Compile with:
 * g++ -o standalone_docking_test standalone_docking_test.cpp \
 *     -I../include -I/usr/include/wx-3.2 \
 *     -L../build/src/docking -ldocking \
 *     -lwx_gtk3u_core-3.2 -lwx_baseu-3.2 \
 *     -std=c++17
 */

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

// Include docking headers
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/FloatingDockContainer.h"
#include "docking/PerspectiveManager.h"

using namespace ads;

// Simple test frame
class TestFrame : public wxFrame {
public:
    TestFrame() : wxFrame(nullptr, wxID_ANY, "Docking System Test", 
                         wxDefaultPosition, wxSize(1200, 800)) {
        
        // Create main panel
        wxPanel* mainPanel = new wxPanel(this);
        
        // Create dock manager
        m_dockManager = new DockManager(mainPanel);
        ConfigureDockManager();
        
        // Create docked widgets
        CreateDockedWidgets();
        
        // Create menus
        CreateMenus();
        
        // Layout
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_dockManager, 1, wxEXPAND);
        mainPanel->SetSizer(sizer);
        
        // Status bar
        CreateStatusBar(2);
        SetStatusText("Docking system ready", 0);
    }
    
private:
    DockManager* m_dockManager;
    
    void ConfigureDockManager() {
        m_dockManager->setConfigFlag(DockManager::OpaqueSplitterResize, true);
        m_dockManager->setConfigFlag(DockManager::DragPreviewIsDynamic, true);
        m_dockManager->setConfigFlag(DockManager::DragPreviewShowsContentPixmap, true);
        m_dockManager->setConfigFlag(DockManager::DragPreviewHasWindowFrame, true);
        m_dockManager->setConfigFlag(DockManager::DockAreaHasCloseButton, true);
        m_dockManager->setConfigFlag(DockManager::DockAreaHasTabsMenuButton, true);
        m_dockManager->setConfigFlag(DockManager::TabCloseButtonIsToolButton, false);
        m_dockManager->setConfigFlag(DockManager::AllTabsHaveCloseButton, true);
        
        // Auto-hide config
        m_dockManager->setAutoHideConfigFlag(DockManager::AutoHideButtonCheckable, true);
        m_dockManager->setAutoHideConfigFlag(DockManager::AutoHideButtonTogglesArea, true);
    }
    
    void CreateDockedWidgets() {
        // Main view (center)
        auto* mainView = new DockWidget("Main View", m_dockManager);
        wxTextCtrl* mainText = new wxTextCtrl(mainView, wxID_ANY, 
            "Main Content Area\n\nThis is the central widget that cannot be closed.\n\n"
            "Try:\n"
            "- Dragging other panels around\n"
            "- Creating floating windows\n"
            "- Using auto-hide (pin/unpin)\n"
            "- Saving and loading layouts",
            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        mainView->setWidget(mainText);
        mainView->setFeature(DockWidget::DockWidgetClosable, false);
        m_dockManager->addDockWidget(CenterDockWidgetArea, mainView);
        
        // Properties panel (right)
        auto* propPanel = new DockWidget("Properties", m_dockManager);
        wxListCtrl* propList = new wxListCtrl(propPanel, wxID_ANY, 
            wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
        propList->AppendColumn("Property");
        propList->AppendColumn("Value");
        
        // Add sample properties
        long idx = propList->InsertItem(0, "Name");
        propList->SetItem(idx, 1, "Object1");
        idx = propList->InsertItem(1, "Type");
        propList->SetItem(idx, 1, "Mesh");
        idx = propList->InsertItem(2, "Vertices");
        propList->SetItem(idx, 1, "1234");
        
        propPanel->setWidget(propList);
        propPanel->setFeature(DockWidget::DockWidgetPinnable, true);
        propPanel->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));
        m_dockManager->addDockWidget(RightDockWidgetArea, propPanel);
        
        // Tree view (left)
        auto* treePanel = new DockWidget("Scene Tree", m_dockManager);
        wxTreeCtrl* tree = new wxTreeCtrl(treePanel, wxID_ANY);
        wxTreeItemId root = tree->AddRoot("Scene");
        wxTreeItemId group1 = tree->AppendItem(root, "Group 1");
        tree->AppendItem(group1, "Object 1");
        tree->AppendItem(group1, "Object 2");
        wxTreeItemId group2 = tree->AppendItem(root, "Group 2");
        tree->AppendItem(group2, "Object 3");
        tree->AppendItem(group2, "Object 4");
        tree->ExpandAll();
        
        treePanel->setWidget(tree);
        treePanel->setFeature(DockWidget::DockWidgetPinnable, true);
        treePanel->setIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_MENU));
        m_dockManager->addDockWidget(LeftDockWidgetArea, treePanel);
        
        // Output panel (bottom)
        auto* outputPanel = new DockWidget("Output", m_dockManager);
        wxTextCtrl* output = new wxTextCtrl(outputPanel, wxID_ANY,
            "Application started\nDocking system initialized\n",
            wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
        outputPanel->setWidget(output);
        outputPanel->setFeature(DockWidget::DockWidgetPinnable, true);
        outputPanel->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
        m_dockManager->addDockWidget(BottomDockWidgetArea, outputPanel);
        
        // Tool panel (tabbed with tree)
        auto* toolPanel = new DockWidget("Tools", m_dockManager);
        wxPanel* tools = new wxPanel(toolPanel);
        wxBoxSizer* toolSizer = new wxBoxSizer(wxVERTICAL);
        toolSizer->Add(new wxButton(tools, wxID_ANY, "Select"), 0, wxEXPAND | wxALL, 2);
        toolSizer->Add(new wxButton(tools, wxID_ANY, "Move"), 0, wxEXPAND | wxALL, 2);
        toolSizer->Add(new wxButton(tools, wxID_ANY, "Rotate"), 0, wxEXPAND | wxALL, 2);
        toolSizer->Add(new wxButton(tools, wxID_ANY, "Scale"), 0, wxEXPAND | wxALL, 2);
        tools->SetSizer(toolSizer);
        
        toolPanel->setWidget(tools);
        toolPanel->setFeature(DockWidget::DockWidgetPinnable, true);
        toolPanel->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_MENU));
        m_dockManager->addDockWidget(CenterDockWidgetArea, toolPanel, treePanel->dockAreaWidget());
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
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_RESET_LAYOUT, "Reset Layout");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_MANAGE_PERSPECTIVES, "Manage Perspectives...");
        menuBar->Append(viewMenu, "&View");
        
        // Help menu
        wxMenu* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, "&About");
        menuBar->Append(helpMenu, "&Help");
        
        SetMenuBar(menuBar);
        
        // Bind events
        Bind(wxEVT_MENU, &TestFrame::OnExit, this, wxID_EXIT);
        Bind(wxEVT_MENU, &TestFrame::OnAbout, this, wxID_ABOUT);
        Bind(wxEVT_MENU, &TestFrame::OnSaveLayout, this, ID_SAVE_LAYOUT);
        Bind(wxEVT_MENU, &TestFrame::OnLoadLayout, this, ID_LOAD_LAYOUT);
        Bind(wxEVT_MENU, &TestFrame::OnResetLayout, this, ID_RESET_LAYOUT);
        Bind(wxEVT_MENU, &TestFrame::OnManagePerspectives, this, ID_MANAGE_PERSPECTIVES);
    }
    
    void OnExit(wxCommandEvent&) {
        Close(true);
    }
    
    void OnAbout(wxCommandEvent&) {
        wxMessageBox("Docking System Test\n\n"
                    "Demonstrates the wxWidgets advanced docking system.\n\n"
                    "Features:\n"
                    "- Drag and drop docking\n"
                    "- Floating windows\n"
                    "- Auto-hide panels\n"
                    "- Save/load layouts\n"
                    "- Perspectives",
                    "About", wxOK | wxICON_INFORMATION);
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
                SetStatusText("Layout saved: " + dlg.GetPath(), 0);
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
                    SetStatusText("Layout loaded: " + dlg.GetPath(), 0);
                }
            }
        }
    }
    
    void OnResetLayout(wxCommandEvent&) {
        if (wxMessageBox("Reset to default layout?", "Confirm", 
                        wxYES_NO | wxICON_QUESTION) == wxYES) {
            m_dockManager->hideManagerAndFloatingContainers();
            CreateDockedWidgets();
            SetStatusText("Layout reset to default", 0);
        }
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

// Application class
class TestApp : public wxApp {
public:
    virtual bool OnInit() override {
        TestFrame* frame = new TestFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);
