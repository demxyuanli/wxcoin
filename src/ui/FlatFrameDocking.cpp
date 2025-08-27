#include "FlatFrameDocking.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "docking/DockArea.h"
#include "docking/FloatingDockContainer.h"
#include "docking/AutoHideContainer.h"
#include <wx/textctrl.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

using namespace ads;

// Event table
wxBEGIN_EVENT_TABLE(FlatFrameDocking, FlatFrame)
    EVT_MENU(ID_DOCKING_SAVE_LAYOUT, FlatFrameDocking::OnDockingSaveLayout)
    EVT_MENU(ID_DOCKING_LOAD_LAYOUT, FlatFrameDocking::OnDockingLoadLayout)
    EVT_MENU(ID_DOCKING_RESET_LAYOUT, FlatFrameDocking::OnDockingResetLayout)
    EVT_MENU(ID_DOCKING_MANAGE_PERSPECTIVES, FlatFrameDocking::OnDockingManagePerspectives)
    EVT_MENU(ID_DOCKING_TOGGLE_AUTOHIDE, FlatFrameDocking::OnDockingToggleAutoHide)
    
    // View panel events
    EVT_MENU(ID_VIEW_PROPERTIES, FlatFrameDocking::OnViewShowHidePanel)
    EVT_MENU(ID_VIEW_OBJECT_TREE, FlatFrameDocking::OnViewShowHidePanel)
    EVT_MENU(ID_VIEW_OUTPUT, FlatFrameDocking::OnViewShowHidePanel)
    EVT_MENU(ID_VIEW_TOOLBOX, FlatFrameDocking::OnViewShowHidePanel)
    
    EVT_UPDATE_UI(ID_VIEW_PROPERTIES, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_OBJECT_TREE, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_OUTPUT, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_TOOLBOX, FlatFrameDocking::OnUpdateUI)
wxEND_EVENT_TABLE()

FlatFrameDocking::FlatFrameDocking(const wxString& title, const wxPoint& pos, const wxSize& size)
    : FlatFrame(title, pos, size)
    , m_dockManager(nullptr)
    , m_propertyDock(nullptr)
    , m_objectTreeDock(nullptr)
    , m_canvasDock(nullptr)
    , m_outputDock(nullptr)
    , m_toolboxDock(nullptr)
    , m_outputCtrl(nullptr)
{
    // Initialize docking system after base class construction
    InitializeDockingLayout();
}

FlatFrameDocking::~FlatFrameDocking() {
    // DockManager will be deleted by its parent (this window)
}

void FlatFrameDocking::InitializeDockingLayout() {
    // Create main panel to hold the dock manager
    wxPanel* mainPanel = new wxPanel(this);
    
    // Create dock manager on the main panel
    m_dockManager = new DockManager(mainPanel);
    
    // Configure dock manager
    ConfigureDockManager();
    
    // Create docking layout
    CreateDockingLayout();
    
    // Create docking-specific menus
    CreateDockingMenus();
    
    // Set up main panel sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_dockManager, 1, wxEXPAND);
    mainPanel->SetSizer(mainSizer);
    
    // Add main panel to frame
    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(mainPanel, 1, wxEXPAND);
    SetSizer(frameSizer);
    
    // Initialize status bar and other UI elements from base class
    CreateStatusBar(3);
    SetStatusText("Ready", 0);
}

void FlatFrameDocking::ConfigureDockManager() {
    // Configure dock manager features
    m_dockManager->setConfigFlag(DockManager::OpaqueSplitterResize, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewIsDynamic, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewShowsContentPixmap, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewHasWindowFrame, true);
    m_dockManager->setConfigFlag(DockManager::DockAreaHasCloseButton, true);
    m_dockManager->setConfigFlag(DockManager::DockAreaHasTabsMenuButton, true);
    m_dockManager->setConfigFlag(DockManager::DockAreaHideDisabledButtons, true);
    m_dockManager->setConfigFlag(DockManager::TabCloseButtonIsToolButton, false);
    m_dockManager->setConfigFlag(DockManager::AllTabsHaveCloseButton, true);
    
    // Enable auto-hide feature
    m_dockManager->setAutoHideConfigFlag(DockManager::AutoHideButtonCheckable, true);
    m_dockManager->setAutoHideConfigFlag(DockManager::AutoHideButtonTogglesArea, true);
    m_dockManager->setAutoHideConfigFlag(DockManager::AutoHideHasCloseButton, true);
}

void FlatFrameDocking::CreateDockingLayout() {
    // Create main canvas dock widget (center)
    m_canvasDock = CreateCanvasDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_canvasDock);
    
    // Create object tree dock widget (left)
    m_objectTreeDock = CreateObjectTreeDockWidget();
    m_dockManager->addDockWidget(LeftDockWidgetArea, m_objectTreeDock);
    
    // Create property panel dock widget (right)
    m_propertyDock = CreatePropertyDockWidget();
    m_dockManager->addDockWidget(RightDockWidgetArea, m_propertyDock);
    
    // Create output dock widget (bottom)
    m_outputDock = CreateOutputDockWidget();
    m_dockManager->addDockWidget(BottomDockWidgetArea, m_outputDock);
    
    // Create toolbox dock widget (tabbed with object tree)
    m_toolboxDock = CreateToolboxDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_toolboxDock, m_objectTreeDock->dockAreaWidget());
    
    // Set initial focus to canvas
    m_canvasDock->setAsCurrentTab();
}

DockWidget* FlatFrameDocking::CreateCanvasDockWidget() {
    DockWidget* dock = new DockWidget("3D View", m_dockManager);
    
    // Create canvas (OCCViewer)
    m_canvas = new Canvas(dock);
    dock->setWidget(m_canvas);
    
    // Configure dock widget
    dock->setFeature(DockWidget::DockWidgetClosable, false);  // Canvas should not be closable
    dock->setFeature(DockWidget::DockWidgetMovable, true);
    dock->setFeature(DockWidget::DockWidgetFloatable, true);
    dock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreatePropertyDockWidget() {
    DockWidget* dock = new DockWidget("Properties", m_dockManager);
    
    // Create property panel
    m_propertyPanel = new PropertyPanel(dock);
    dock->setWidget(m_propertyPanel);
    
    // Configure dock widget
    dock->setFeature(DockWidget::DockWidgetClosable, true);
    dock->setFeature(DockWidget::DockWidgetMovable, true);
    dock->setFeature(DockWidget::DockWidgetFloatable, true);
    dock->setFeature(DockWidget::DockWidgetPinnable, true);  // Can be auto-hidden
    dock->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreateObjectTreeDockWidget() {
    DockWidget* dock = new DockWidget("Object Tree", m_dockManager);
    
    // Create object tree panel
    m_objectTreePanel = new ObjectTreePanel(dock);
    dock->setWidget(m_objectTreePanel);
    
    // Configure dock widget
    dock->setFeature(DockWidget::DockWidgetClosable, true);
    dock->setFeature(DockWidget::DockWidgetMovable, true);
    dock->setFeature(DockWidget::DockWidgetFloatable, true);
    dock->setFeature(DockWidget::DockWidgetPinnable, true);  // Can be auto-hidden
    dock->setIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreateOutputDockWidget() {
    DockWidget* dock = new DockWidget("Output", m_dockManager);
    
    // Create output text control
    wxTextCtrl* output = new wxTextCtrl(dock, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    
    // Add initial content
    output->SetDefaultStyle(wxTextAttr(*wxBLACK));
    output->AppendText("Application started.\n");
    output->AppendText("Docking system initialized.\n");
    
    dock->setWidget(output);
    
    // Configure dock widget
    dock->setFeature(DockWidget::DockWidgetClosable, true);
    dock->setFeature(DockWidget::DockWidgetMovable, true);
    dock->setFeature(DockWidget::DockWidgetFloatable, true);
    dock->setFeature(DockWidget::DockWidgetPinnable, true);  // Can be auto-hidden
    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
    
    // Store output control for later use
    m_outputCtrl = output;
    
    return dock;
}

DockWidget* FlatFrameDocking::CreateToolboxDockWidget() {
    DockWidget* dock = new DockWidget("Toolbox", m_dockManager);
    
    // Create toolbox panel
    wxPanel* toolbox = new wxPanel(dock);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Add tool buttons
    wxButton* selectTool = new wxButton(toolbox, wxID_ANY, "Select");
    wxButton* moveTool = new wxButton(toolbox, wxID_ANY, "Move");
    wxButton* rotateTool = new wxButton(toolbox, wxID_ANY, "Rotate");
    wxButton* scaleTool = new wxButton(toolbox, wxID_ANY, "Scale");
    wxButton* measureTool = new wxButton(toolbox, wxID_ANY, "Measure");
    
    sizer->Add(selectTool, 0, wxEXPAND | wxALL, 2);
    sizer->Add(moveTool, 0, wxEXPAND | wxALL, 2);
    sizer->Add(rotateTool, 0, wxEXPAND | wxALL, 2);
    sizer->Add(scaleTool, 0, wxEXPAND | wxALL, 2);
    sizer->Add(measureTool, 0, wxEXPAND | wxALL, 2);
    sizer->AddStretchSpacer();
    
    toolbox->SetSizer(sizer);
    dock->setWidget(toolbox);
    
    // Configure dock widget
    dock->setFeature(DockWidget::DockWidgetClosable, true);
    dock->setFeature(DockWidget::DockWidgetMovable, true);
    dock->setFeature(DockWidget::DockWidgetFloatable, true);
    dock->setFeature(DockWidget::DockWidgetPinnable, true);  // Can be auto-hidden
    dock->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_MENU));
    
    return dock;
}

void FlatFrameDocking::CreateDockingMenus() {
    // Get or create View menu
    wxMenuBar* menuBar = GetMenuBar();
    if (!menuBar) return;
    
    wxMenu* viewMenu = nullptr;
    int viewMenuIndex = menuBar->FindMenu("View");
    if (viewMenuIndex != wxNOT_FOUND) {
        viewMenu = menuBar->GetMenu(viewMenuIndex);
    } else {
        viewMenu = new wxMenu();
        menuBar->Append(viewMenu, "&View");
    }
    
    // Add separator before docking items
    viewMenu->AppendSeparator();
    
    // Add docking menu items
    viewMenu->Append(ID_DOCKING_SAVE_LAYOUT, "Save &Layout...\tCtrl+L", 
                     "Save current docking layout");
    viewMenu->Append(ID_DOCKING_LOAD_LAYOUT, "Load L&ayout...\tCtrl+Shift+L", 
                     "Load saved docking layout");
    viewMenu->Append(ID_DOCKING_RESET_LAYOUT, "&Reset Layout", 
                     "Reset to default docking layout");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_DOCKING_MANAGE_PERSPECTIVES, "&Manage Perspectives...", 
                     "Manage saved layout perspectives");
    viewMenu->Append(ID_DOCKING_TOGGLE_AUTOHIDE, "Toggle &Auto-hide\tCtrl+H", 
                     "Toggle auto-hide for current panel");
}

void FlatFrameDocking::SaveDockingLayout(const wxString& filename) {
    wxString state;
    m_dockManager->saveState(state);
    
    wxFile file(filename, wxFile::write);
    if (file.IsOpened()) {
        file.Write(state);
        file.Close();
        
        appendMessage("Layout saved to: " + filename);
    } else {
        wxMessageBox("Failed to save layout file", "Error", wxOK | wxICON_ERROR);
    }
}

void FlatFrameDocking::LoadDockingLayout(const wxString& filename) {
    wxFile file(filename, wxFile::read);
    if (file.IsOpened()) {
        wxString state;
        file.ReadAll(&state);
        file.Close();
        
        if (m_dockManager->restoreState(state)) {
            appendMessage("Layout loaded from: " + filename);
        } else {
            wxMessageBox("Failed to restore layout", "Error", wxOK | wxICON_ERROR);
        }
    } else {
        wxMessageBox("Failed to open layout file", "Error", wxOK | wxICON_ERROR);
    }
}

void FlatFrameDocking::ResetDockingLayout() {
    // Hide all current widgets
    m_dockManager->hideManagerAndFloatingContainers();
    
    // Recreate default layout
    CreateDockingLayout();
    
    appendMessage("Layout reset to default");
}

// Event handlers
void FlatFrameDocking::OnDockingSaveLayout(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Save Docking Layout", "", "layout.xml",
                     "XML files (*.xml)|*.xml",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() == wxID_OK) {
        SaveDockingLayout(dlg.GetPath());
    }
}

void FlatFrameDocking::OnDockingLoadLayout(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Load Docking Layout", "", "",
                     "XML files (*.xml)|*.xml",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dlg.ShowModal() == wxID_OK) {
        LoadDockingLayout(dlg.GetPath());
    }
}

void FlatFrameDocking::OnDockingResetLayout(wxCommandEvent& event) {
    int answer = wxMessageBox("Reset to default layout?", "Confirm Reset",
                              wxYES_NO | wxICON_QUESTION);
    if (answer == wxYES) {
        ResetDockingLayout();
    }
}

void FlatFrameDocking::OnDockingManagePerspectives(wxCommandEvent& event) {
    PerspectiveDialog dlg(this, m_dockManager->perspectiveManager());
    dlg.ShowModal();
}

void FlatFrameDocking::OnDockingToggleAutoHide(wxCommandEvent& event) {
    // Get current focused dock widget
    auto widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        if (widget->isCurrentTab()) {
            bool isAutoHide = widget->isAutoHide();
            widget->setAutoHide(!isAutoHide);
            appendMessage(wxString::Format("%s auto-hide %s", 
                       widget->windowTitle(),
                       isAutoHide ? "disabled" : "enabled"));
            break;
        }
    }
}

void FlatFrameDocking::OnViewShowHidePanel(wxCommandEvent& event) {
    // Handle show/hide for dock widgets
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