#include "FlatFrameDocking.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "ui/PerformancePanel.h"
#include "docking/DockArea.h"
#include "docking/DockingIntegration.h"
#include "EventLogger.h"
#include <wx/msgdlg.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <memory>

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
      m_canvasDock(nullptr),
      m_outputDock(nullptr),
      m_toolboxDock(nullptr),
      m_outputCtrl(nullptr)
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
    
    // Enable auto-hide feature
    m_dockManager->setConfigFlag(ads::DockManager::AutoHideFeatureEnabled, true);
    
    // Enable opaque splitter resize
    m_dockManager->setConfigFlag(ads::DockManager::OpaqueSplitterResize, true);
    
    // Enable XML compression
    m_dockManager->setConfigFlag(ads::DockManager::XmlCompressionEnabled, true);
    
    // Set proper focus highlighting
    m_dockManager->setConfigFlag(ads::DockManager::FocusHighlighting, true);
    
    // Enable equal split visibility
    m_dockManager->setConfigFlag(ads::DockManager::EqualSplitOnInsertion, true);
}

void FlatFrameDocking::CreateDockingLayout() {
    if (!m_dockManager) return;
    
    // Create dock widgets for our panels
    m_canvasDock = CreateCanvasDockWidget();
    m_propertyDock = CreatePropertyDockWidget();
    m_objectTreeDock = CreateObjectTreeDockWidget();
    m_outputDock = CreateOutputDockWidget();
    
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
    
    // Add output to the bottom
    if (m_outputDock) {
        m_dockManager->addDockWidget(ads::BottomDockWidgetArea, m_outputDock);
    }
}

ads::DockWidget* FlatFrameDocking::CreatePropertyDockWidget() {
    if (!m_propertyPanel) return nullptr;
    
    auto* dock = new ads::DockWidget("Properties");
    dock->setWidget(m_propertyPanel);
    dock->setIcon(wxArtProvider::GetIcon(wxART_LIST_VIEW));
    dock->setFeature(ads::DockWidget::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidget::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidget::DockWidgetFloatable, true);
    dock->setMinimumSize(wxSize(200, 100));
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateObjectTreeDockWidget() {
    if (!m_objectTreePanel) return nullptr;
    
    auto* dock = new ads::DockWidget("Object Tree");
    dock->setWidget(m_objectTreePanel);
    dock->setIcon(wxArtProvider::GetIcon(wxART_FOLDER_OPEN));
    dock->setFeature(ads::DockWidget::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidget::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidget::DockWidgetFloatable, true);
    dock->setMinimumSize(wxSize(200, 100));
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateCanvasDockWidget() {
    if (!m_canvas) return nullptr;
    
    auto* dock = new ads::DockWidget("3D View");
    dock->setWidget(m_canvas);
    dock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE));
    dock->setFeature(ads::DockWidget::DockWidgetClosable, false);
    dock->setMinimumSize(wxSize(300, 200));
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateOutputDockWidget() {
    // Create output control if not exists
    if (!m_outputCtrl) {
        m_outputCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
        m_outputCtrl->SetBackgroundColour(wxColour(30, 30, 30));
        m_outputCtrl->SetForegroundColour(wxColour(200, 200, 200));
        
        wxFont font(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
        m_outputCtrl->SetFont(font);
    }
    
    auto* dock = new ads::DockWidget("Output");
    dock->setWidget(m_outputCtrl);
    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION));
    dock->setFeature(ads::DockWidget::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidget::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidget::DockWidgetFloatable, true);
    dock->setMinimumSize(wxSize(200, 100));
    
    return dock;
}

ads::DockWidget* FlatFrameDocking::CreateToolboxDockWidget() {
    // Create a placeholder toolbox panel
    auto* toolbox = new wxPanel(this);
    toolbox->SetBackgroundColour(wxColour(45, 45, 45));
    
    auto* dock = new ads::DockWidget("Toolbox");
    dock->setWidget(toolbox);
    dock->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE));
    dock->setFeature(ads::DockWidget::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidget::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidget::DockWidgetFloatable, true);
    dock->setMinimumSize(wxSize(150, 100));
    
    return dock;
}

void FlatFrameDocking::SaveDockingLayout(const wxString& filename) {
    if (!m_dockManager) return;
    
    try {
        m_dockManager->savePerspective(filename.ToStdString());
        wxMessageBox("Layout saved successfully!", "Save Layout", 
                    wxOK | wxICON_INFORMATION, this);
    } catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Failed to save layout: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR, this);
    }
}

void FlatFrameDocking::LoadDockingLayout(const wxString& filename) {
    if (!m_dockManager) return;
    
    try {
        m_dockManager->loadPerspective(filename.ToStdString());
        wxMessageBox("Layout loaded successfully!", "Load Layout", 
                    wxOK | wxICON_INFORMATION, this);
    } catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Failed to load layout: %s", e.what()),
                    "Error", wxOK | wxICON_ERROR, this);
    }
}

void FlatFrameDocking::ResetDockingLayout() {
    if (!m_dockManager) return;
    
    // Clear current layout
    m_dockManager->clear();
    
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
    windowMenu->AppendSeparator();
    
    // Add docking features
    windowMenu->Append(ID_DOCKING_TOGGLE_AUTOHIDE, "Toggle Auto-Hide");
    windowMenu->Append(ID_DOCKING_MANAGE_PERSPECTIVES, "Manage Perspectives...");
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
            
        case ID_VIEW_OUTPUT:
            if (m_outputDock) {
                m_outputDock->toggleView();
            }
            break;
            
        case ID_VIEW_TOOLBOX:
            if (m_toolboxDock) {
                m_toolboxDock->toggleView();
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
            
        case ID_VIEW_OUTPUT:
            if (m_outputDock) {
                event.Check(m_outputDock->isVisible());
            }
            break;
            
        case ID_VIEW_TOOLBOX:
            if (m_toolboxDock) {
                event.Check(m_toolboxDock->isVisible());
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