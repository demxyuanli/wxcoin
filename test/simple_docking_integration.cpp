/**
 * @brief Simple docking integration test
 * 
 * This is a simplified version that doesn't require modifying CMakeLists.txt
 * or dealing with complex dependencies.
 * 
 * Compile with:
 * g++ -o simple_docking simple_docking_integration.cpp \
 *     -I../include `wx-config --cxxflags` \
 *     -L../build/src/docking -ldocking \
 *     `wx-config --libs` -std=c++17
 */

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/stc/stc.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/PerspectiveManager.h"

using namespace ads;

// Simple frame that integrates docking
class SimpleDockingFrame : public wxFrame {
public:
    SimpleDockingFrame() 
        : wxFrame(nullptr, wxID_ANY, "Simple Docking Integration", 
                  wxDefaultPosition, wxSize(1024, 768)) {
        
        // Create main panel
        wxPanel* mainPanel = new wxPanel(this);
        
        // Create dock manager
        m_dockManager = new DockManager(mainPanel);
        ConfigureDocking();
        
        // Create a simple layout
        CreateSimpleLayout();
        
        // Setup main panel
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_dockManager, 1, wxEXPAND);
        mainPanel->SetSizer(sizer);
        
        // Create minimal menu
        CreateMenu();
        
        // Status bar
        CreateStatusBar();
        SetStatusText("Docking system integrated successfully");
    }
    
private:
    DockManager* m_dockManager;
    
    void ConfigureDocking() {
        // Basic configuration
        m_dockManager->setConfigFlag(DockManager::OpaqueSplitterResize, true);
        m_dockManager->setConfigFlag(DockManager::DragPreviewIsDynamic, true);
        m_dockManager->setConfigFlag(DockManager::DockAreaHasCloseButton, true);
        m_dockManager->setConfigFlag(DockManager::DockAreaHasTabsMenuButton, true);
        m_dockManager->setConfigFlag(DockManager::AllTabsHaveCloseButton, true);
    }
    
    void CreateSimpleLayout() {
        // Main editor/view
        DockWidget* mainDock = new DockWidget("Main View", m_dockManager);
        wxStyledTextCtrl* editor = new wxStyledTextCtrl(mainDock);
        editor->SetText("// Main content area\n"
                       "// This demonstrates docking integration\n\n"
                       "void example() {\n"
                       "    // Your code here\n"
                       "}\n");
        editor->SetLexer(wxSTC_LEX_CPP);
        editor->StyleClearAll();
        editor->SetMarginType(0, wxSTC_MARGIN_NUMBER);
        editor->SetMarginWidth(0, 50);
        mainDock->setWidget(editor);
        mainDock->setFeature(DockWidget::DockWidgetClosable, false);
        m_dockManager->addDockWidget(CenterDockWidgetArea, mainDock);
        
        // Side panel - file tree
        DockWidget* treeDock = new DockWidget("Files", m_dockManager);
        wxTreeCtrl* tree = new wxTreeCtrl(treeDock);
        wxTreeItemId root = tree->AddRoot("Project");
        tree->AppendItem(root, "src");
        tree->AppendItem(root, "include");
        tree->AppendItem(root, "docs");
        tree->ExpandAll();
        treeDock->setWidget(tree);
        treeDock->setFeature(DockWidget::DockWidgetPinnable, true);
        m_dockManager->addDockWidget(LeftDockWidgetArea, treeDock);
        
        // Properties panel
        DockWidget* propDock = new DockWidget("Properties", m_dockManager);
        wxListCtrl* props = new wxListCtrl(propDock, wxID_ANY,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxLC_REPORT);
        props->AppendColumn("Property", wxLIST_FORMAT_LEFT, 120);
        props->AppendColumn("Value", wxLIST_FORMAT_LEFT, 180);
        propDock->setWidget(props);
        propDock->setFeature(DockWidget::DockWidgetPinnable, true);
        m_dockManager->addDockWidget(RightDockWidgetArea, propDock);
        
        // Output panel
        DockWidget* outputDock = new DockWidget("Output", m_dockManager);
        wxTextCtrl* output = new wxTextCtrl(outputDock, wxID_ANY,
                                           "Build output will appear here...\n",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_MULTILINE | wxTE_READONLY);
        outputDock->setWidget(output);
        outputDock->setFeature(DockWidget::DockWidgetPinnable, true);
        m_dockManager->addDockWidget(BottomDockWidgetArea, outputDock);
    }
    
    void CreateMenu() {
        wxMenuBar* menuBar = new wxMenuBar();
        
        // File menu
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_EXIT, "E&xit");
        menuBar->Append(fileMenu, "&File");
        
        // View menu
        wxMenu* viewMenu = new wxMenu();
        viewMenu->Append(ID_SAVE_PERSPECTIVE, "Save Perspective...\tCtrl+S");
        viewMenu->Append(ID_MANAGE_PERSPECTIVES, "Manage Perspectives...");
        viewMenu->AppendSeparator();
        viewMenu->Append(ID_RESET_LAYOUT, "Reset Layout");
        menuBar->Append(viewMenu, "&View");
        
        SetMenuBar(menuBar);
        
        // Bind events
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnSavePerspective, this, ID_SAVE_PERSPECTIVE);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnManagePerspectives, this, ID_MANAGE_PERSPECTIVES);
        Bind(wxEVT_MENU, &SimpleDockingFrame::OnResetLayout, this, ID_RESET_LAYOUT);
    }
    
    void OnSavePerspective(wxCommandEvent&) {
        wxString name = wxGetTextFromUser("Enter perspective name:", "Save Perspective");
        if (!name.IsEmpty()) {
            m_dockManager->perspectiveManager()->savePerspective(name);
            SetStatusText("Perspective saved: " + name);
        }
    }
    
    void OnManagePerspectives(wxCommandEvent&) {
        PerspectiveDialog dlg(this, m_dockManager->perspectiveManager());
        dlg.ShowModal();
    }
    
    void OnResetLayout(wxCommandEvent&) {
        m_dockManager->hideManagerAndFloatingContainers();
        CreateSimpleLayout();
        SetStatusText("Layout reset");
    }
    
    enum {
        ID_SAVE_PERSPECTIVE = wxID_HIGHEST + 1,
        ID_MANAGE_PERSPECTIVES,
        ID_RESET_LAYOUT
    };
};

// Application
class SimpleDockingApp : public wxApp {
public:
    virtual bool OnInit() override {
        SimpleDockingFrame* frame = new SimpleDockingFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(SimpleDockingApp);