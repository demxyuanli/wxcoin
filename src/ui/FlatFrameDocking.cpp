#include "FlatFrameDocking.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/menu.h>

wxBEGIN_EVENT_TABLE(FlatFrameDocking, FlatFrame)
    EVT_MENU(ID_DOCKING_SAVE_LAYOUT, FlatFrameDocking::OnDockingSaveLayout)
    EVT_MENU(ID_DOCKING_LOAD_LAYOUT, FlatFrameDocking::OnDockingLoadLayout)
    EVT_MENU(ID_DOCKING_RESET_LAYOUT, FlatFrameDocking::OnDockingResetLayout)
    EVT_MENU(ID_DOCKING_MANAGE_PERSPECTIVES, FlatFrameDocking::OnDockingManagePerspectives)
    EVT_MENU(ID_DOCKING_TOGGLE_AUTOHIDE, FlatFrameDocking::OnDockingToggleAutoHide)
    EVT_MENU_RANGE(ID_VIEW_PROPERTIES, ID_VIEW_TOOLBOX, FlatFrameDocking::OnViewShowHidePanel)
    EVT_UPDATE_UI_RANGE(ID_VIEW_PROPERTIES, ID_VIEW_TOOLBOX, FlatFrameDocking::OnUpdateUI)
wxEND_EVENT_TABLE()

FlatFrameDocking::FlatFrameDocking(const wxString& title, const wxPoint& pos, const wxSize& size)
    : FlatFrame(title, pos, size),
      m_dockManager(nullptr),
      m_propertyDock(nullptr),
      m_objectTreeDock(nullptr),
      m_canvasDock(nullptr)
{
    // Initialize docking system
    InitializeDockingLayout();
    
    // Create docking-specific menus
    CreateDockingMenus();
}

FlatFrameDocking::~FlatFrameDocking() {
    if (m_dockManager) {
        delete m_dockManager;
        m_dockManager = nullptr;
    }
}

bool FlatFrameDocking::Destroy() {
    // Clean up dock manager before destroying the frame
    if (m_dockManager) {
        delete m_dockManager;
        m_dockManager = nullptr;
    }
    
    return FlatFrame::Destroy();
}

void FlatFrameDocking::InitializeDockingLayout() {
    // Create dock manager with this frame as parent
    m_dockManager = new ads::DockManager(this);
    
    // Configure dock manager
    ConfigureDockManager();
    
    // Create the docking layout
    CreateDockingLayout();
}

void FlatFrameDocking::ConfigureDockManager() {
    if (!m_dockManager) return;
    
    // Use available configuration flags
    // Note: The actual flags depend on your docking library implementation
}

void FlatFrameDocking::CreateDockingLayout() {
    if (!m_dockManager) return;
    
    // Create dock widgets for our panels
    m_canvasDock = CreateCanvasDockWidget();
    m_propertyDock = CreatePropertyDockWidget();
    m_objectTreeDock = CreateObjectTreeDockWidget();
    
    // Add widgets to dock manager
    // Note: The actual API depends on your docking library implementation
}

ads::DockWidget* FlatFrameDocking::CreatePropertyDockWidget() {
    // Create a dock widget for properties
    // Note: Implementation depends on your docking library
    return nullptr;
}

ads::DockWidget* FlatFrameDocking::CreateObjectTreeDockWidget() {
    // Create a dock widget for object tree
    // Note: Implementation depends on your docking library
    return nullptr;
}

ads::DockWidget* FlatFrameDocking::CreateCanvasDockWidget() {
    // Create a dock widget for canvas
    // Note: Implementation depends on your docking library
    return nullptr;
}

ads::DockWidget* FlatFrameDocking::CreateOutputDockWidget() {
    // Create a dock widget for output
    // Note: Implementation depends on your docking library
    return nullptr;
}

ads::DockWidget* FlatFrameDocking::CreateToolboxDockWidget() {
    // Create a dock widget for toolbox
    // Note: Implementation depends on your docking library
    return nullptr;
}

void FlatFrameDocking::SaveDockingLayout(const wxString& filename) {
    // Save layout implementation
    wxMessageBox("Save layout functionality not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::LoadDockingLayout(const wxString& filename) {
    // Load layout implementation
    wxMessageBox("Load layout functionality not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::ResetDockingLayout() {
    // Reset layout implementation
    wxMessageBox("Reset layout functionality not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::CreateDockingMenus() {
    // Get the menubar
    wxMenuBar* menuBar = GetMenuBar();
    if (!menuBar) return;
    
    // Find or create Window menu
    wxMenu* windowMenu = nullptr;
    int windowMenuIndex = menuBar->FindMenu("Window");
    
    if (windowMenuIndex == wxNOT_FOUND) {
        windowMenu = new wxMenu();
        menuBar->Append(windowMenu, "Window");
    } else {
        windowMenu = menuBar->GetMenu(windowMenuIndex);
    }
    
    // Add separator if menu already has items
    if (windowMenu->GetMenuItemCount() > 0) {
        windowMenu->AppendSeparator();
    }
    
    // Add docking menu items
    windowMenu->Append(ID_DOCKING_SAVE_LAYOUT, "Save Layout...\tCtrl+Shift+S");
    windowMenu->Append(ID_DOCKING_LOAD_LAYOUT, "Load Layout...\tCtrl+Shift+O");
    windowMenu->Append(ID_DOCKING_RESET_LAYOUT, "Reset Layout");
}

// Event handlers
void FlatFrameDocking::OnDockingSaveLayout(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Save Docking Layout", "", "",
                    "Layout files (*.xml)|*.xml|All files (*.*)|*.*",
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
                    
    if (dlg.ShowModal() == wxID_OK) {
        SaveDockingLayout(dlg.GetPath());
    }
}

void FlatFrameDocking::OnDockingLoadLayout(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Load Docking Layout", "", "",
                    "Layout files (*.xml)|*.xml|All files (*.*)|*.*",
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
                    
    if (dlg.ShowModal() == wxID_OK) {
        LoadDockingLayout(dlg.GetPath());
    }
}

void FlatFrameDocking::OnDockingResetLayout(wxCommandEvent& event) {
    int result = wxMessageBox("Are you sure you want to reset the layout to default?",
                             "Reset Layout", wxYES_NO | wxICON_QUESTION, this);
                             
    if (result == wxYES) {
        ResetDockingLayout();
    }
}

void FlatFrameDocking::OnDockingManagePerspectives(wxCommandEvent& event) {
    wxMessageBox("Perspective manager not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::OnDockingToggleAutoHide(wxCommandEvent& event) {
    wxMessageBox("Auto-hide toggle not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::OnViewShowHidePanel(wxCommandEvent& event) {
    // Handle view show/hide events
}

void FlatFrameDocking::OnUpdateUI(wxUpdateUIEvent& event) {
    // Handle UI update events
}