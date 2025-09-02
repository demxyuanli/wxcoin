#include "FlatFrameDocking.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "docking/DockArea.h"
#include "EventLogger.h"
#include <wx/msgdlg.h>
#include <wx/artprov.h>
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
    // Ensure panels are created
    EnsurePanelsCreated();
    
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
    
    // Enable features using the correct API
    m_dockManager->setConfigFlag(ads::DockAreaHasCloseButton, true);
    m_dockManager->setConfigFlag(ads::DockAreaHasUndockButton, true);
    m_dockManager->setConfigFlag(ads::ActiveTabHasCloseButton, true);
    m_dockManager->setConfigFlag(ads::FocusHighlighting, true);
}

void FlatFrameDocking::CreateDockingLayout() {
    if (!m_dockManager) return;
    
    // Create dock widgets for our panels
    m_canvasDock = CreateCanvasDockWidget();
    m_propertyDock = CreatePropertyDockWidget();
    m_objectTreeDock = CreateObjectTreeDockWidget();
    
    // Add canvas as the central widget
    if (m_canvasDock) {
        m_dockManager->addDockWidget(ads::CenterDockWidgetArea, m_canvasDock);
    }
    
    // Add object tree to the left
    if (m_objectTreeDock) {
        m_dockManager->addDockWidget(ads::LeftDockWidgetArea, m_objectTreeDock);
    }
    
    // Add properties to the right
    if (m_propertyDock) {
        m_dockManager->addDockWidget(ads::RightDockWidgetArea, m_propertyDock);
    }
}

ads::DockWidget* FlatFrameDocking::CreatePropertyDockWidget() {
    PropertyPanel* panel = GetPropertyPanel();
    if (!panel) return nullptr;
    
    auto* dock = new ads::DockWidget("Properties");
    dock->setWidget(panel);
    dock->setIcon(wxArtProvider::GetIcon(wxART_LIST_VIEW));
    dock->setFeature(ads::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidgetFloatable, true);
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateObjectTreeDockWidget() {
    ObjectTreePanel* panel = GetObjectTreePanel();
    if (!panel) return nullptr;
    
    auto* dock = new ads::DockWidget("Object Tree");
    dock->setWidget(panel);
    dock->setIcon(wxArtProvider::GetIcon(wxART_FOLDER_OPEN));
    dock->setFeature(ads::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidgetFloatable, true);
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateCanvasDockWidget() {
    Canvas* canvas = GetCanvas();
    if (!canvas) return nullptr;
    
    auto* dock = new ads::DockWidget("3D View");
    dock->setWidget(canvas);
    dock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE));
    dock->setFeature(ads::DockWidgetClosable, false);
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateOutputDockWidget() {
    // Create output control if not exists
    wxTextCtrl* outputCtrl = GetMessageOutput();
    if (!outputCtrl) {
        outputCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
        outputCtrl->SetBackgroundColour(wxColour(30, 30, 30));
        outputCtrl->SetForegroundColour(wxColour(200, 200, 200));
        
        wxFont font(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        outputCtrl->SetFont(font);
    }
    
    auto* dock = new ads::DockWidget("Output");
    dock->setWidget(outputCtrl);
    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION));
    dock->setFeature(ads::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidgetFloatable, true);
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateToolboxDockWidget() {
    // Create a placeholder toolbox panel
    auto* toolbox = new wxPanel(this);
    toolbox->SetBackgroundColour(wxColour(45, 45, 45));
    
    auto* dock = new ads::DockWidget("Toolbox");
    dock->setWidget(toolbox);
    dock->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE));
    dock->setFeature(ads::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidgetFloatable, true);
    
    return dock;
}

void FlatFrameDocking::SaveDockingLayout(const wxString& filename) {
    if (!m_dockManager) return;
    
    try {
        wxString xmlData;
        m_dockManager->saveState(xmlData);
        
        // Save to file
        wxFile file(filename, wxFile::write);
        if (file.IsOpened()) {
            file.Write(xmlData);
            file.Close();
            wxMessageBox("Layout saved successfully!", "Save Layout", 
                        wxOK | wxICON_INFORMATION, this);
        }
    } catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Failed to save layout: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR, this);
    }
}

void FlatFrameDocking::LoadDockingLayout(const wxString& filename) {
    if (!m_dockManager) return;
    
    try {
        wxFile file(filename, wxFile::read);
        if (file.IsOpened()) {
            wxString xmlData;
            file.ReadAll(&xmlData);
            file.Close();
            
            if (m_dockManager->restoreState(xmlData)) {
                wxMessageBox("Layout loaded successfully!", "Load Layout", 
                            wxOK | wxICON_INFORMATION, this);
            }
        }
    } catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Failed to load layout: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR, this);
    }
}

void FlatFrameDocking::ResetDockingLayout() {
    if (!m_dockManager) return;
    
    // Remove all dock widgets
    auto widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        m_dockManager->removeDockWidget(widget);
    }
    
    // Recreate default layout
    CreateDockingLayout();
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
    windowMenu->AppendSeparator();
    
    // Add view toggles
    windowMenu->AppendCheckItem(ID_VIEW_PROPERTIES, "Properties\tF4");
    windowMenu->AppendCheckItem(ID_VIEW_OBJECT_TREE, "Object Tree\tF3");
    windowMenu->AppendCheckItem(ID_VIEW_OUTPUT, "Output\tF6");
    windowMenu->AppendCheckItem(ID_VIEW_TOOLBOX, "Toolbox\tF7");
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
    // TODO: Implement perspective manager dialog
    wxMessageBox("Perspective manager not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::OnDockingToggleAutoHide(wxCommandEvent& event) {
    // TODO: Implement auto-hide toggle
    wxMessageBox("Auto-hide toggle not yet implemented", "Info", 
                wxOK | wxICON_INFORMATION, this);
}

void FlatFrameDocking::OnViewShowHidePanel(wxCommandEvent& event) {
    int id = event.GetId();
    
    switch (id) {
        case ID_VIEW_PROPERTIES:
            if (m_propertyDock) {
                m_propertyDock->toggleView();
            }
            break;
            
        case ID_VIEW_OBJECT_TREE:
            if (m_objectTreeDock) {
                m_objectTreeDock->toggleView();
            }
            break;
            
        default:
            // Ignore other events
            break;
    }
}

void FlatFrameDocking::OnUpdateUI(wxUpdateUIEvent& event) {
    int id = event.GetId();
    
    switch (id) {
        case ID_VIEW_PROPERTIES:
            if (m_propertyDock) {
                event.Check(m_propertyDock->isVisible());
            }
            break;
            
        case ID_VIEW_OBJECT_TREE:
            if (m_objectTreeDock) {
                event.Check(m_objectTreeDock->isVisible());
            }
            break;
            
        default:
            // Ignore other events
            break;
    }
}

wxWindow* FlatFrameDocking::GetMainWorkArea() const {
    // Return the dock manager's container widget as the main work area
    return m_dockManager ? m_dockManager->containerWidget() : nullptr;
}