#include "FlatFrameDocking.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "ui/PerformancePanel.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "InputManager.h"
#include "docking/DockArea.h"
#include "docking/FloatingDockContainer.h"
#include "docking/AutoHideContainer.h"
#include "docking/DockLayoutConfig.h"
#include <wx/textctrl.h>
#include <wx/artprov.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/file.h>
#include <wx/stattext.h>

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
    EVT_MENU(ID_VIEW_MESSAGE, FlatFrameDocking::OnViewShowHidePanel)
    EVT_MENU(ID_VIEW_PERFORMANCE, FlatFrameDocking::OnViewShowHidePanel)
    EVT_MENU(ID_VIEW_OUTPUT, FlatFrameDocking::OnViewShowHidePanel)  // Backward compatibility
    EVT_MENU(ID_VIEW_TOOLBOX, FlatFrameDocking::OnViewShowHidePanel)
    
    EVT_UPDATE_UI(ID_VIEW_PROPERTIES, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_OBJECT_TREE, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_MESSAGE, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_PERFORMANCE, FlatFrameDocking::OnUpdateUI)
    EVT_UPDATE_UI(ID_VIEW_OUTPUT, FlatFrameDocking::OnUpdateUI)  // Backward compatibility
    EVT_UPDATE_UI(ID_VIEW_TOOLBOX, FlatFrameDocking::OnUpdateUI)
    
    // Override base class size event to ensure docking system controls layout
    EVT_SIZE(FlatFrameDocking::onSize)
wxEND_EVENT_TABLE()

FlatFrameDocking::FlatFrameDocking(const wxString& title, const wxPoint& pos, const wxSize& size)
    : FlatFrame(title, pos, size)
    , m_dockManager(nullptr)
    , m_workAreaPanel(nullptr)
    , m_propertyDock(nullptr)
    , m_objectTreeDock(nullptr)
    , m_canvasDock(nullptr)
    , m_messageDock(nullptr)
    , m_performanceDock(nullptr)
    , m_toolboxDock(nullptr)
    , m_outputCtrl(nullptr)
{
    // IMPORTANT: At this point, base class has already called InitializeUI
    // which may have created ModernDockAdapter if IsUsingDockingSystem() returned false
    // We need to clean that up first
    
    // Initialize docking system after base class construction
    InitializeDockingLayout();
    
    // IMPORTANT: EnsurePanelsCreated must be called AFTER InitializeDockingLayout
    // because InitializeDockingLayout hides existing panels from base class
    // and we need to ensure our panels are properly created and connected
    EnsurePanelsCreated();
}

FlatFrameDocking::~FlatFrameDocking() {
    // DockManager will be deleted by its parent (mainPanel)
    // Just clear our reference
    m_dockManager = nullptr;
}

bool FlatFrameDocking::Destroy() {
    // Clear our reference to dock manager
    // The actual cleanup will happen through normal parent-child destruction
    m_dockManager = nullptr;
    
    // Call base class Destroy
    return FlatFrame::Destroy();
}

void FlatFrameDocking::InitializeDockingLayout() {
    // IMPORTANT: We need to remove/hide any panels created by the base class
    // and replace them with our docking system
    
    wxLogDebug("InitializeDockingLayout: Starting");
    
    // Get the ribbon from base class
    FlatUIBar* ribbon = GetUIBar();
    
    // Hide any existing children that might have been created by base class
    wxWindowList& children = GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        // Keep FlatUIBar (ribbon) and status bar, hide everything else
        if (child != ribbon && !child->IsKindOf(CLASSINFO(wxStatusBar))) {
            child->Hide();
        }
    }
    
    // Clear any existing sizer to start fresh
    SetSizer(nullptr);
    
    // Create a new main sizer for the frame
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
    
    // Add the ribbon if it exists
    if (ribbon) {
        mainSizer->Add(ribbon, 0, wxEXPAND);
    }
    
    // Create a panel for the main work area (between FlatUIBar and StatusBar)
    m_workAreaPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    
    // Create dock manager to manage the work area
    m_dockManager = new DockManager(m_workAreaPanel);
    
    // Configure dock manager
    ConfigureDockManager();
    
    // Create docking layout
    CreateDockingLayout();
    
    // Create docking-specific menus
    CreateDockingMenus();
    
    // Set up work area panel sizer
    wxBoxSizer* workAreaSizer = new wxBoxSizer(wxVERTICAL);
    workAreaSizer->Add(m_dockManager->containerWidget(), 1, wxEXPAND);
    m_workAreaPanel->SetSizer(workAreaSizer);
    
    // Add the work area panel to the main sizer
    mainSizer->Add(m_workAreaPanel, 1, wxEXPAND);
    
    // Use the same status bar creation as base class
    addStatusBar();
    if (auto* bar = GetFlatUIStatusBar()) {
        bar->SetFieldsCount(3);
        bar->SetStatusText("Ready - Docking Layout Active", 0);
        bar->EnableProgressGauge(false);
        bar->SetGaugeRange(100);
        bar->SetGaugeValue(0);
    }
    
    // Force layout update
    Layout();
    m_workAreaPanel->Layout();
    
    // Ensure our docking system has focus
    if (m_canvasDock && m_canvasDock->widget()) {
        m_canvasDock->widget()->SetFocus();
    }
}

void FlatFrameDocking::ConfigureDockManager() {
    // Configure dock manager features
    m_dockManager->setConfigFlag(OpaqueSplitterResize, true);
    m_dockManager->setConfigFlag(DockAreaHasCloseButton, true);
    m_dockManager->setConfigFlag(TabCloseButtonIsToolButton, false);
    m_dockManager->setConfigFlag(AllTabsHaveCloseButton, true);
    m_dockManager->setConfigFlag(FocusHighlighting, true);
    
    // Configure default layout sizes
    DockLayoutConfig layoutConfig;
    layoutConfig.leftAreaWidth = 300;      // Width for object tree and properties
    layoutConfig.bottomAreaHeight = 150;   // Height for message/performance panel
    layoutConfig.usePercentage = false;
    m_dockManager->setLayoutConfig(layoutConfig);
    
    // Note: Auto-hide configuration is done through the AutoHideManager
    // which is managed internally by DockManager
}

void FlatFrameDocking::CreateDockingLayout() {
    // 1. Create main canvas dock widget (center)
    m_canvasDock = CreateCanvasDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_canvasDock);
    
    // 2. Create object tree dock widget (left-top)
    m_objectTreeDock = CreateObjectTreeDockWidget();
    DockArea* leftTopArea = m_dockManager->addDockWidget(LeftDockWidgetArea, m_objectTreeDock);
    
    // 3. Create property panel dock widget (left-bottom) - split below object tree
    m_propertyDock = CreatePropertyDockWidget();
    m_dockManager->addDockWidget(BottomDockWidgetArea, m_propertyDock, leftTopArea);
    
    // 4. Create message dock widget (bottom) - renamed from output
    m_messageDock = CreateMessageDockWidget();
    DockArea* bottomArea = m_dockManager->addDockWidget(BottomDockWidgetArea, m_messageDock);
    
    // 5. Create performance dock widget (bottom tab with message)
    m_performanceDock = CreatePerformanceDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_performanceDock, bottomArea);
    
    // Set initial focus to canvas
    m_canvasDock->setAsCurrentTab();
    
    // CRITICAL: After all panels are docked, ensure Canvas has all necessary connections
    // This is especially important if Canvas was reparented
    Canvas* canvas = GetCanvas();
    if (canvas && canvas->getInputManager()) {
        // Ensure MouseHandler is connected
        MouseHandler* mouseHandler = dynamic_cast<MouseHandler*>(canvas->getInputManager()->getMouseHandler());
        if (!mouseHandler && GetObjectTreePanel() && GetPropertyPanel()) {
            // Recreate MouseHandler if it's missing
            mouseHandler = new MouseHandler(canvas, GetObjectTreePanel(), GetPropertyPanel(), canvas->getCommandManager());
            canvas->getInputManager()->setMouseHandler(mouseHandler);
            
            // Also ensure NavigationController is set
            NavigationController* navController = new NavigationController(canvas, canvas->getSceneManager());
            canvas->getInputManager()->setNavigationController(navController);
            mouseHandler->setNavigationController(navController);
        }
        
        // Canvas will refresh itself when needed, no need to force it here
    }
}

DockWidget* FlatFrameDocking::CreateCanvasDockWidget() {
    DockWidget* dock = new DockWidget("3D View", m_dockManager->containerWidget());
    
    // Use existing canvas from base class
    Canvas* canvas = GetCanvas();
    if (!canvas) {
        // Only create new if base class doesn't have one
        canvas = new Canvas(dock);
    } else {
        // For OpenGL canvas, we need to be very careful with reparenting
        // First, ensure the canvas is not in any sizer
        if (canvas->GetContainingSizer()) {
            canvas->GetContainingSizer()->Detach(canvas);
        }
        
        // Hide temporarily to avoid rendering issues during reparent
        canvas->Hide();
        
        // Note: wxGLCanvas manages its own context internally
        // We don't need to manually save/restore the GL context during reparenting
        // The canvas will handle this when it's refreshed
        
        // Now reparent
        canvas->Reparent(dock);
        
        // Show again after reparenting
        canvas->Show();
        
        // Force a refresh to reinitialize OpenGL if needed
        canvas->Refresh(false);  // Use false to avoid immediate repaint
    }
    
    dock->setWidget(canvas);
    
    // Configure dock widget
    dock->setFeature(DockWidgetClosable, false);  // Canvas should not be closable
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);
    dock->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreatePropertyDockWidget() {
    DockWidget* dock = new DockWidget("Properties", m_dockManager->containerWidget());
    
    // Use existing property panel from base class if available, otherwise create new
    PropertyPanel* propertyPanel = GetPropertyPanel();
    if (!propertyPanel) {
        propertyPanel = new PropertyPanel(dock);
    } else {
        // Remove from existing sizer if any
        if (propertyPanel->GetContainingSizer()) {
            propertyPanel->GetContainingSizer()->Detach(propertyPanel);
        }
        // Reparent existing panel to the dock widget
        propertyPanel->Reparent(dock);
    }
    dock->setWidget(propertyPanel);
    
    // Configure dock widget
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);

    dock->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreateObjectTreeDockWidget() {
    DockWidget* dock = new DockWidget("Object Tree", m_dockManager->containerWidget());
    
    // Use existing object tree panel from base class if available, otherwise create new
    ObjectTreePanel* objectTreePanel = GetObjectTreePanel();
    if (!objectTreePanel) {
        objectTreePanel = new ObjectTreePanel(dock);
    } else {
        // Remove from existing sizer if any
        if (objectTreePanel->GetContainingSizer()) {
            objectTreePanel->GetContainingSizer()->Detach(objectTreePanel);
        }
        // Reparent existing panel to the dock widget
        objectTreePanel->Reparent(dock);
    }
    dock->setWidget(objectTreePanel);
    
    // Configure dock widget
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);

    dock->setIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreateMessageDockWidget() {
    DockWidget* dock = new DockWidget("Message", m_dockManager->containerWidget());
    
    // Use existing message output from base class if available, otherwise create new
    wxTextCtrl* output = GetMessageOutput();
    if (!output) {
        output = new wxTextCtrl(dock, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
        
        // Add initial content
        output->SetDefaultStyle(wxTextAttr(*wxBLACK));
        output->AppendText("Application started.\n");
        output->AppendText("Docking system initialized.\n");
    } else {
        // Remove from existing sizer if any
        if (output->GetContainingSizer()) {
            output->GetContainingSizer()->Detach(output);
        }
        // Reparent existing output to the dock widget
        output->Reparent(dock);
    }
    
    dock->setWidget(output);
    
    // Configure dock widget
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);

    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
    
    // Store output control for later use
    m_outputCtrl = output;
    
    return dock;
}

DockWidget* FlatFrameDocking::CreatePerformanceDockWidget() {
    DockWidget* dock = new DockWidget("Performance", m_dockManager->containerWidget());
    
    // Create a container panel first
    wxPanel* container = new wxPanel(dock);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create PerformancePanel with the container as parent
    PerformancePanel* perfPanel = new PerformancePanel(container);
    perfPanel->SetMinSize(wxSize(360, 140));
    
    // Add performance panel to container
    sizer->Add(perfPanel, 1, wxEXPAND);
    container->SetSizer(sizer);
    
    // Set the container as the dock widget
    dock->setWidget(container);
    
    // Configure dock widget
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, false);  // Performance usually stays in bottom
    dock->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
    
    return dock;
}

DockWidget* FlatFrameDocking::CreateToolboxDockWidget() {
    DockWidget* dock = new DockWidget("Toolbox", m_dockManager->containerWidget());
    
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
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);

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
    
    // Add panel visibility items
    viewMenu->AppendCheckItem(ID_VIEW_OBJECT_TREE, "Object Tree\tCtrl+Alt+O", 
                              "Show/hide object tree panel");
    viewMenu->AppendCheckItem(ID_VIEW_PROPERTIES, "Properties\tCtrl+Alt+P", 
                              "Show/hide properties panel");
    viewMenu->AppendCheckItem(ID_VIEW_MESSAGE, "Message\tCtrl+Alt+M", 
                              "Show/hide message output panel");
    viewMenu->AppendCheckItem(ID_VIEW_PERFORMANCE, "Performance\tCtrl+Alt+F", 
                              "Show/hide performance monitor panel");
    
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
    // Remove all dock widgets
    auto widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        m_dockManager->removeDockWidget(widget);
    }
    
    // Clear references
    m_propertyDock = nullptr;
    m_objectTreeDock = nullptr;
    m_canvasDock = nullptr;
    m_messageDock = nullptr;
    m_performanceDock = nullptr;
    m_toolboxDock = nullptr;
    
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
    ads::PerspectiveDialog dlg(this, m_dockManager->perspectiveManager());
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
                       widget->title(),
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
            
        case ID_VIEW_MESSAGE:
        case ID_VIEW_OUTPUT:  // Backward compatibility
            if (m_messageDock) {
                m_messageDock->toggleView();
            }
            break;
            
        case ID_VIEW_PERFORMANCE:
            if (m_performanceDock) {
                m_performanceDock->toggleView();
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
            
        case ID_VIEW_MESSAGE:
        case ID_VIEW_OUTPUT:  // Backward compatibility
            if (m_messageDock) {
                event.Check(m_messageDock->isVisible());
            }
            break;
            
        case ID_VIEW_PERFORMANCE:
            if (m_performanceDock) {
                event.Check(m_performanceDock->isVisible());
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

void FlatFrameDocking::onSize(wxSizeEvent& event) {
    // IMPORTANT: We handle our own layout through the docking system
    // Do NOT call base class onSize which might interfere with our layout
    
    // Just let the event propagate to child windows (docking system)
    event.Skip();
    
    // NOTE: Removed Refresh() call here as it was causing performance issues
    // The docking system will handle its own layout updates
}

wxWindow* FlatFrameDocking::GetMainWorkArea() {
    // Return the dock container widget as the main work area
    return m_dockManager ? m_dockManager->containerWidget() : m_workAreaPanel;
}