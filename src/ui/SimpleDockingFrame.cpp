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
#include "docking/DockManager.h"
#include "docking/DockWidget.h"
#include "docking/PerspectiveManager.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"

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
        // Main canvas
        DockWidget* canvasDock = new DockWidget("3D View", m_dockManager->containerWidget());
        Canvas* canvas = new Canvas(canvasDock);
        canvasDock->setWidget(canvas);
        canvasDock->setFeature(DockWidgetClosable, false);
        canvasDock->setFeature(DockWidgetMovable, true);
        canvasDock->setFeature(DockWidgetFloatable, true);
        canvasDock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU));
        m_dockManager->addDockWidget(CenterDockWidgetArea, canvasDock);
        
        // Properties panel
        DockWidget* propDock = new DockWidget("Properties", m_dockManager->containerWidget());
        PropertyPanel* props = new PropertyPanel(propDock);
        propDock->setWidget(props);
        propDock->setFeature(DockWidgetClosable, true);
        propDock->setFeature(DockWidgetMovable, true);
        propDock->setFeature(DockWidgetFloatable, true);
        propDock->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));
        m_dockManager->addDockWidget(RightDockWidgetArea, propDock);
        
        // Object tree
        DockWidget* treeDock = new DockWidget("Object Tree", m_dockManager->containerWidget());
        ObjectTreePanel* tree = new ObjectTreePanel(treeDock);
        treeDock->setWidget(tree);
        treeDock->setFeature(DockWidgetClosable, true);
        treeDock->setFeature(DockWidgetMovable, true);
        treeDock->setFeature(DockWidgetFloatable, true);
        treeDock->setIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_MENU));
        m_dockManager->addDockWidget(LeftDockWidgetArea, treeDock);
        
        // Output window
        DockWidget* outputDock = new DockWidget("Output", m_dockManager->containerWidget());
        wxTextCtrl* output = new wxTextCtrl(outputDock, wxID_ANY,
                                           "Application started\nDocking initialized\n",
                                           wxDefaultPosition, wxDefaultSize,
                                           wxTE_MULTILINE | wxTE_READONLY);
        outputDock->setWidget(output);
        outputDock->setFeature(DockWidgetClosable, true);
        outputDock->setFeature(DockWidgetMovable, true);
        outputDock->setFeature(DockWidgetFloatable, true);
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
