#include "FlatFrameDocking.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "ui/PerformancePanel.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "InputManager.h"
#include "logger/Logger.h"
#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/AutoHideContainer.h"
#include "docking/DockLayoutConfig.h"
#include "DockLayoutConfigListener.h"
#include "CommandListenerManager.h"
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
EVT_MENU(ID_DOCKING_CONFIGURE_LAYOUT, FlatFrameDocking::OnDockingConfigureLayout)
EVT_MENU(ID_DOCKING_ADJUST_LAYOUT, FlatFrameDocking::OnDockingAdjustLayout)

// View panel events
EVT_MENU(ID_VIEW_PROPERTIES, FlatFrameDocking::OnViewShowHidePanel)
EVT_MENU(ID_VIEW_OBJECT_TREE, FlatFrameDocking::OnViewShowHidePanel)
EVT_MENU(ID_VIEW_MESSAGE, FlatFrameDocking::OnViewShowHidePanel)
EVT_MENU(ID_VIEW_PERFORMANCE, FlatFrameDocking::OnViewShowHidePanel)


EVT_UPDATE_UI(ID_VIEW_PROPERTIES, FlatFrameDocking::OnUpdateUI)
EVT_UPDATE_UI(ID_VIEW_OBJECT_TREE, FlatFrameDocking::OnUpdateUI)
EVT_UPDATE_UI(ID_VIEW_MESSAGE, FlatFrameDocking::OnUpdateUI)
EVT_UPDATE_UI(ID_VIEW_PERFORMANCE, FlatFrameDocking::OnUpdateUI)

// Override base class size event to ensure docking system controls layout
EVT_SIZE(FlatFrameDocking::onSize)
EVT_TIMER(ID_RESIZE_TIMER, FlatFrameDocking::OnResizeTimer)
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
    , m_outputCtrl(nullptr)
    , m_resizeTimer(nullptr)
    , m_pendingResizeSize(0, 0)
    , m_resizePending(false)
{
    // IMPORTANT: At this point, base class has already called InitializeUI
    // which may have created ModernDockAdapter if IsUsingDockingSystem() returned false
    // We need to clean that up first

    // Initialize resize timer for optimized layout adjustments
    m_resizeTimer = new wxTimer(this, ID_RESIZE_TIMER);

    // Initialize docking system after base class construction
    InitializeDockingLayout();

    // IMPORTANT: EnsurePanelsCreated must be called AFTER InitializeDockingLayout
    // because InitializeDockingLayout hides existing panels from base class
    // and we need to ensure our panels are properly created and connected
    EnsurePanelsCreated();
    
    // Register dock layout config listener
    RegisterDockLayoutConfigListener();
}

FlatFrameDocking::~FlatFrameDocking() {
    // Stop and delete resize timer
    if (m_resizeTimer) {
        if (m_resizeTimer->IsRunning()) {
            m_resizeTimer->Stop();
        }
        delete m_resizeTimer;
        m_resizeTimer = nullptr;
    }

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

    // Set modern docking style - use FLAT style similar to FlatUIBar
    ads::DockArea::SetDockStyle(ads::DockStyle::FLAT);

    // Get the ribbon from base class
    FlatUIBar* ribbon = GetUIBar();

    // Hide any existing children that might have been created by base class
    wxWindowList& children = GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        // Keep FlatUIBar (ribbon) and status bar, hide everything else
        if (child != ribbon &&
            !child->IsKindOf(CLASSINFO(wxStatusBar)) &&
            child != GetFlatUIStatusBar()) {
            child->Hide();
        }
    }

    // Don't use SetSizer(nullptr) as it can break base class references
    // Instead, get the existing sizer and clear it
    wxSizer* oldSizer = GetSizer();
    if (oldSizer) {
        oldSizer->Clear(false);
    }

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

    // Add the work area panel to the main sizer with border margins
    mainSizer->Add(m_workAreaPanel, 1, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, 2);

    // Get status bar from base class (should already exist from BorderlessFrameLogic constructor)
    FlatUIStatusBar* statusBar = GetFlatUIStatusBar();
    if (!statusBar) {
        LOG_WRN_S("FlatUIStatusBar not found in InitializeDockingLayout, this should not happen!");
        // As a fallback, create one (though this indicates a problem in initialization order)
        statusBar = new FlatUIStatusBar(this);
    }

    // Configure status bar
    if (statusBar) {
        statusBar->SetFieldsCount(3);
        statusBar->SetStatusText("Ready - Docking Layout Active", 0);
        statusBar->EnableProgressGauge(false);
        statusBar->SetGaugeRange(100);
        statusBar->SetGaugeValue(0);

        // Ensure status bar is added to the main sizer at the bottom with border margins
        if (mainSizer && !mainSizer->GetItem(statusBar)) {
            mainSizer->Add(statusBar, 0, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, 1);
        }

        // Ensure status bar is visible
        statusBar->Show();
    }

    // Force layout update
    Layout();
    m_workAreaPanel->Layout();

    // Additional insurance: Apply layout config after final layout update
    // This ensures 20/80 layout is maintained even after wxWidgets layout operations
    wxLogDebug("InitializeDockingLayout: Applying final layout config insurance");
    if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
        if (ads::DockContainerWidget* container = dynamic_cast<ads::DockContainerWidget*>(containerWidget)) {
            // First try the standard applyLayoutConfig
            container->applyLayoutConfig();
            wxLogDebug("InitializeDockingLayout: Final layout config applied");

            // Then force manual adjustment as backup
            wxSize finalSize = container->GetSize();
            if (finalSize.x > 100 && finalSize.y > 100) {
                wxLogDebug("InitializeDockingLayout: Applying manual 20/80 layout correction");
                ApplyManual2080Layout(container);
                wxLogDebug("InitializeDockingLayout: Manual layout correction applied");
            }
        }
    }

    // Log final dock areas after all initialization
    LogCurrentDockAreas("FINAL after initialization");

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
    layoutConfig.leftAreaWidth = 200;      // Minimum width for left area (15/85 layout)
    layoutConfig.bottomAreaHeight = 150;   // Height for message/performance panel (fallback)
    layoutConfig.leftAreaPercent = 15;     // 15% for left dock area (15/85 layout)
    layoutConfig.bottomAreaPercent = 20;   // 20% for bottom dock area (15/85 layout)
    layoutConfig.topAreaPercent = 0;       // No top area for clean 15/85 layout
    layoutConfig.rightAreaPercent = 0;     // No right area for clean 15/85 layout
    layoutConfig.usePercentage = true;     // Use percentage-based layout

    wxLogDebug("ConfigureDockManager: Setting 15/85 layout config:");
    wxLogDebug("  - usePercentage: %s", layoutConfig.usePercentage ? "true" : "false");
    wxLogDebug("  - leftAreaPercent: %d%%", layoutConfig.leftAreaPercent);
    wxLogDebug("  - bottomAreaPercent: %d%%", layoutConfig.bottomAreaPercent);
    wxLogDebug("  - topAreaPercent: %d%%", layoutConfig.topAreaPercent);
    wxLogDebug("  - rightAreaPercent: %d%%", layoutConfig.rightAreaPercent);
    wxLogDebug("  - showLeftArea: %s", layoutConfig.showLeftArea ? "true" : "false");
    wxLogDebug("  - showBottomArea: %s", layoutConfig.showBottomArea ? "true" : "false");
    wxLogDebug("  - showTopArea: %s", layoutConfig.showTopArea ? "true" : "false");
    wxLogDebug("  - showRightArea: %s", layoutConfig.showRightArea ? "true" : "false");

    m_dockManager->setLayoutConfig(layoutConfig);

    // Note: Auto-hide configuration is done through the AutoHideManager
    // which is managed internally by DockManager
}

void FlatFrameDocking::CreateDockingLayout() {
    wxLogDebug("CreateDockingLayout: Starting dock widget creation");

    // Create in order that matches applyLayoutConfig expectations:
    // First create left area, then center, then bottom

    // 1. Create object tree dock widget (left)
    m_objectTreeDock = CreateObjectTreeDockWidget();
    DockArea* leftTopArea = m_dockManager->addDockWidget(LeftDockWidgetArea, m_objectTreeDock);
    wxLogDebug("CreateDockingLayout: Object tree dock created");

    // 2. Create property panel dock widget (left-bottom) - split below object tree
    m_propertyDock = CreatePropertyDockWidget();
    m_dockManager->addDockWidget(BottomDockWidgetArea, m_propertyDock, leftTopArea);
    wxLogDebug("CreateDockingLayout: Property dock created");

    // 3. Create main canvas dock widget (center) - this should create the right side
    m_canvasDock = CreateCanvasDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_canvasDock);
    wxLogDebug("CreateDockingLayout: Canvas dock created");

    // 4. Create message dock widget (bottom)
    m_messageDock = CreateMessageDockWidget();
    DockArea* bottomArea = m_dockManager->addDockWidget(BottomDockWidgetArea, m_messageDock);
    wxLogDebug("CreateDockingLayout: Message dock created");

    // 5. Create performance dock widget (bottom tab with message)
    m_performanceDock = CreatePerformanceDockWidget();
    m_dockManager->addDockWidget(CenterDockWidgetArea, m_performanceDock, bottomArea);
    wxLogDebug("CreateDockingLayout: Performance dock created");

    // Log current dock areas and their sizes before applying layout config
    LogCurrentDockAreas("BEFORE applyLayoutConfig");

    // Set initial focus to canvas
    m_canvasDock->setAsCurrentTab();

    // Force a layout update before applying config
    if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
        containerWidget->Layout();
        wxLogDebug("CreateDockingLayout: Forced layout update before config");
    }

    // CRITICAL: Apply layout configuration after all dock widgets are created
    // This ensures the 20/80 layout is properly applied
    if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
        if (ads::DockContainerWidget* container = dynamic_cast<ads::DockContainerWidget*>(containerWidget)) {
            wxLogDebug("CreateDockingLayout: Applying layout config...");
            container->applyLayoutConfig();
            wxLogDebug("CreateDockingLayout: Layout config applied");

            // Force another layout update after config application
            container->Layout();
            container->Refresh();
            wxLogDebug("CreateDockingLayout: Forced layout update after config");

            // Check container size before manual adjustment
            wxSize currentContainerSize = container->GetSize();
            wxLogDebug("CreateDockingLayout: Container size before manual adjustment: %dx%d",
                       currentContainerSize.x, currentContainerSize.y);

            if (currentContainerSize.x > 100 && currentContainerSize.y > 100) {
                // DIRECT APPROACH: Manually set splitter positions for 20/80 layout
                wxLogDebug("CreateDockingLayout: Applying manual 20/80 splitter positions...");
                ApplyManual2080Layout(container);
                wxLogDebug("CreateDockingLayout: Manual layout applied");

                // Force additional layout updates
                container->Layout();
                container->Refresh();
                wxLogDebug("CreateDockingLayout: Additional layout update after manual adjustment");
            } else {
                wxLogDebug("CreateDockingLayout: Container too small (%dx%d), skipping manual adjustment",
                           currentContainerSize.x, currentContainerSize.y);
            }
        }
    }

    // Log dock areas and their sizes after applying layout config
    LogCurrentDockAreas("AFTER applyLayoutConfig");

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
    }
    else {
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
    }
    else {
        // Remove from existing sizer if any
        if (propertyPanel->GetContainingSizer()) {
            propertyPanel->GetContainingSizer()->Detach(propertyPanel);
        }
        // Reparent existing panel to the dock widget
        propertyPanel->Reparent(dock);
    }
    dock->setWidget(propertyPanel);

    // Set minimum size for the property panel widget
    if (propertyPanel) {
        propertyPanel->SetMinSize(wxSize(150, -1));
    }

    // Configure dock widget
    dock->setFeature(DockWidgetClosable, true);
    dock->setFeature(DockWidgetMovable, true);
    dock->setFeature(DockWidgetFloatable, true);

    dock->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));

    return dock;
}

DockWidget* FlatFrameDocking::CreateObjectTreeDockWidget() {
    DockWidget* dock = new DockWidget("Tree View", m_dockManager->containerWidget());

    // Use existing object tree panel from base class if available, otherwise create new
    ObjectTreePanel* objectTreePanel = GetObjectTreePanel();
    if (!objectTreePanel) {
        objectTreePanel = new ObjectTreePanel(dock);
    }
    else {
        // Remove from existing sizer if any
        if (objectTreePanel->GetContainingSizer()) {
            objectTreePanel->GetContainingSizer()->Detach(objectTreePanel);
        }
        // Reparent existing panel to the dock widget
        objectTreePanel->Reparent(dock);
    }
    dock->setWidget(objectTreePanel);

    // Set minimum size for the widget to ensure left panel cannot be compressed below 200px
    if (objectTreePanel) {
        objectTreePanel->SetMinSize(wxSize(200, -1));
    }

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
    }
    else {
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
    // Since all docking functionality is now in the ribbon's Docking page,
    // we don't need to create any traditional menu items here.
    // This method is kept empty for compatibility.
}

void FlatFrameDocking::SaveDockingLayout(const wxString& filename) {
    wxString state;
    m_dockManager->saveState(state);

    wxFile file(filename, wxFile::write);
    if (file.IsOpened()) {
        file.Write(state);
        file.Close();

        appendMessage("Layout saved to: " + filename);
    }
    else {
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
        }
        else {
            wxMessageBox("Failed to restore layout", "Error", wxOK | wxICON_ERROR);
        }
    }
    else {
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

void FlatFrameDocking::OnDockingConfigureLayout(wxCommandEvent& event) {
    if (!m_dockManager) return;

    const DockLayoutConfig& currentConfig = m_dockManager->getLayoutConfig();
    DockLayoutConfig config = currentConfig;  // Make a copy
    DockLayoutConfigDialog dlg(this, config, m_dockManager);

    if (dlg.ShowModal() == wxID_OK) {
        config = dlg.GetConfig();
        m_dockManager->setLayoutConfig(config);

        // Apply the configuration immediately to the main container
        if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
            if (DockContainerWidget* container = dynamic_cast<DockContainerWidget*>(containerWidget)) {
                container->applyLayoutConfig();
            }
        }

        appendMessage("Layout configuration updated and applied");
    }
}

void FlatFrameDocking::OnDockingAdjustLayout(wxCommandEvent& event) {
    wxLogDebug("OnDockingAdjustLayout: Starting dynamic layout adjustment");

    if (!m_dockManager) {
        wxLogDebug("OnDockingAdjustLayout: DockManager is null");
        return;
    }

    // Get the current container size
    if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
        if (ads::DockContainerWidget* container = dynamic_cast<ads::DockContainerWidget*>(containerWidget)) {
            wxSize containerSize = container->GetSize();
            wxLogDebug("OnDockingAdjustLayout: Container size: %dx%d", containerSize.x, containerSize.y);

            // Only proceed if size is reasonable
            if (containerSize.x > 200 && containerSize.y > 150) {
                // Apply dynamic layout adjustment
                ApplyDynamicLayoutAdjustment(container);

                wxLogDebug("OnDockingAdjustLayout: Dynamic layout adjustment completed");
            } else {
                wxLogDebug("OnDockingAdjustLayout: Container size too small, skipping adjustment");
            }
        }
    }
}

void FlatFrameDocking::OnResizeTimer(wxTimerEvent& event) {
    wxLogDebug("OnResizeTimer: Timer fired, processing delayed resize");

    if (m_resizePending && m_dockManager && m_dockManager->containerWidget()) {
        HandleDelayedResize();
        m_resizePending = false;
    }
}

void FlatFrameDocking::HandleDelayedResize() {
    wxLogDebug("HandleDelayedResize: Processing resize for cached size %dx%d",
               m_pendingResizeSize.x, m_pendingResizeSize.y);

    if (!m_dockManager || !m_dockManager->containerWidget()) {
        return;
    }

    // Verify the current size matches our cached size (to avoid unnecessary adjustments)
    wxSize currentSize = m_dockManager->containerWidget()->GetSize();

    // Only proceed if the current size matches our cached pending size
    // This ensures we only adjust once when the resize has actually finished
    if (std::abs(currentSize.x - m_pendingResizeSize.x) <= 5 &&
        std::abs(currentSize.y - m_pendingResizeSize.y) <= 5) {

        wxLogDebug("HandleDelayedResize: Size matches cached size, applying layout adjustment");

        if (ads::DockContainerWidget* container = dynamic_cast<ads::DockContainerWidget*>(m_dockManager->containerWidget())) {
            ApplyDynamicLayoutAdjustmentOptimized(container);
        }

        wxLogDebug("HandleDelayedResize: Layout adjustment completed");
    } else {
        wxLogDebug("HandleDelayedResize: Size changed during timer (%dx%d vs %dx%d), skipping adjustment",
                   currentSize.x, currentSize.y, m_pendingResizeSize.x, m_pendingResizeSize.y);
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
        if (m_messageDock) {
            m_messageDock->toggleView();
        }
        break;

    case ID_VIEW_PERFORMANCE:
        if (m_performanceDock) {
            m_performanceDock->toggleView();
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
        if (m_messageDock) {
            event.Check(m_messageDock->isVisible());
        }
        break;

    case ID_VIEW_PERFORMANCE:
        if (m_performanceDock) {
            event.Check(m_performanceDock->isVisible());
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

    // Let the event propagate to child windows first
    event.Skip();

    // Use timer-based approach to avoid multiple layout adjustments during resize
    if (m_dockManager && m_dockManager->containerWidget() && m_resizeTimer) {
        wxSize newSize = m_dockManager->containerWidget()->GetSize();

        // Only process if size is reasonable (avoid tiny sizes during initialization)
        if (newSize.x > 200 && newSize.y > 150) {
            // Cache the new size for delayed processing
            m_pendingResizeSize = newSize;
            m_resizePending = true;

            // Start/restart the timer (200ms delay to wait for resize to finish)
            if (m_resizeTimer->IsRunning()) {
                m_resizeTimer->Stop();
            }
            m_resizeTimer->Start(200, wxTIMER_ONE_SHOT);

            wxLogDebug("onSize: Resize detected (%dx%d), timer started for delayed adjustment",
                       newSize.x, newSize.y);
        }
    }
}

wxWindow* FlatFrameDocking::GetMainWorkArea() const {
    // Return the dock container widget as the main work area
    return m_dockManager ? m_dockManager->containerWidget() : m_workAreaPanel;
}

void FlatFrameDocking::RegisterDockLayoutConfigListener() {
    // Register the dock layout config listener using base class method
    if (m_dockManager) {
        auto dockLayoutConfigListener = std::make_shared<DockLayoutConfigListener>(m_dockManager);
        RegisterCommandListener(cmd::CommandType::DockLayoutConfig, dockLayoutConfigListener);
    }
}

void FlatFrameDocking::LogCurrentDockAreas(const wxString& context) {
    if (!m_dockManager) {
        wxLogDebug("%s: DockManager is null", context);
        return;
    }

    wxLogDebug("%s: Logging current dock areas:", context);

    // Log container widget size
    if (wxWindow* container = m_dockManager->containerWidget()) {
        wxSize containerSize = container->GetSize();
        wxLogDebug("  - Container size: %dx%d", containerSize.x, containerSize.y);
    }

    // Log specific dock widgets and their sizes
    wxLogDebug("  - Canvas dock widget:");
    if (m_canvasDock && m_canvasDock->widget()) {
        wxSize canvasSize = m_canvasDock->widget()->GetSize();
        wxLogDebug("    - Size: %dx%d", canvasSize.x, canvasSize.y);
    } else {
        wxLogDebug("    - Not available or no widget");
    }

    wxLogDebug("  - Object tree dock widget:");
    if (m_objectTreeDock && m_objectTreeDock->widget()) {
        wxSize treeSize = m_objectTreeDock->widget()->GetSize();
        wxLogDebug("    - Size: %dx%d", treeSize.x, treeSize.y);
    } else {
        wxLogDebug("    - Not available or no widget");
    }

    wxLogDebug("  - Property dock widget:");
    if (m_propertyDock && m_propertyDock->widget()) {
        wxSize propSize = m_propertyDock->widget()->GetSize();
        wxLogDebug("    - Size: %dx%d", propSize.x, propSize.y);
    } else {
        wxLogDebug("    - Not available or no widget");
    }

    wxLogDebug("  - Message dock widget:");
    if (m_messageDock && m_messageDock->widget()) {
        wxSize msgSize = m_messageDock->widget()->GetSize();
        wxLogDebug("    - Size: %dx%d", msgSize.x, msgSize.y);
    } else {
        wxLogDebug("    - Not available or no widget");
    }

    wxLogDebug("  - Performance dock widget:");
    if (m_performanceDock && m_performanceDock->widget()) {
        wxSize perfSize = m_performanceDock->widget()->GetSize();
        wxLogDebug("    - Size: %dx%d", perfSize.x, perfSize.y);
    } else {
        wxLogDebug("    - Not available or no widget");
    }

    // Log current layout config
    const DockLayoutConfig& config = m_dockManager->getLayoutConfig();
    wxLogDebug("  - Current config: usePercentage=%s, left=%d%%, right=%d%%, top=%d%%, bottom=%d%%",
               config.usePercentage ? "true" : "false",
               config.leftAreaPercent, config.rightAreaPercent,
               config.topAreaPercent, config.bottomAreaPercent);

    // Calculate expected sizes based on container size
    if (wxWindow* container = m_dockManager->containerWidget()) {
        wxSize containerSize = container->GetSize();
        if (config.usePercentage && containerSize.x > 0 && containerSize.y > 0) {
            int expectedLeftWidth = (containerSize.x * config.leftAreaPercent) / 100;
            int expectedCanvasWidth = containerSize.x - expectedLeftWidth;
            int expectedBottomHeight = (containerSize.y * config.bottomAreaPercent) / 100;

            wxLogDebug("  - Expected sizes (based on %dx%d container):", containerSize.x, containerSize.y);
            wxLogDebug("    - Left area width: %dpx (%d%%)", expectedLeftWidth, config.leftAreaPercent);
            wxLogDebug("    - Canvas width: %dpx (remaining)", expectedCanvasWidth);
            wxLogDebug("    - Bottom area height: %dpx (%d%%)", expectedBottomHeight, config.bottomAreaPercent);
        }
    }
}

void FlatFrameDocking::ApplyManual2080Layout(ads::DockContainerWidget* container) {
    if (!container) {
        wxLogDebug("ApplyManual2080Layout: Container is null");
        return;
    }

    // Get container size
    wxSize containerSize = container->GetSize();
    wxLogDebug("ApplyManual2080Layout: Container size: %dx%d", containerSize.x, containerSize.y);

    if (containerSize.x <= 100 || containerSize.y <= 100) {
        wxLogDebug("ApplyManual2080Layout: Container too small for meaningful adjustment");
        return;
    }

    // Calculate desired positions
    const ads::DockLayoutConfig& config = m_dockManager->getLayoutConfig();
    int leftWidth = (containerSize.x * config.leftAreaPercent) / 100;
    int bottomHeight = (containerSize.y * config.bottomAreaPercent) / 100;

    // Ensure left area minimum width of 200px (cannot be compressed)
    if (leftWidth < 200) {
        leftWidth = 200;
        wxLogDebug("ApplyManual2080Layout: Enforcing minimum left width of 200px");
    }

    wxLogDebug("ApplyManual2080Layout: Target left width: %dpx (%d%%)", leftWidth, config.leftAreaPercent);
    wxLogDebug("ApplyManual2080Layout: Target bottom height: %dpx (%d%%)", bottomHeight, config.bottomAreaPercent);

    // Strategy 1: Try to find the main vertical splitter first
    wxSplitterWindow* mainSplitter = FindMainVerticalSplitter(container);
    if (mainSplitter) {
        // Set minimum pane size to enforce 200px minimum width for left panel
        mainSplitter->SetMinimumPaneSize(200);
        wxLogDebug("ApplyManual2080Layout: Set minimum pane size to 200px");

        wxLogDebug("ApplyManual2080Layout: Found main vertical splitter, setting sash to %d", leftWidth);
        mainSplitter->SetSashPosition(leftWidth);
        mainSplitter->UpdateSize();

        // Force immediate update for this splitter
        mainSplitter->Layout();
    } else {
        wxLogDebug("ApplyManual2080Layout: No main vertical splitter found, trying alternative approach");
    }

    // Strategy 2: If no main splitter found, try to adjust all vertical splitters
    int adjustedCount = 0;
    wxWindowList& containerChildren = container->GetChildren();
    for (wxWindowList::iterator it = containerChildren.begin(); it != containerChildren.end(); ++it) {
        wxWindow* child = *it;
        adjustedCount += AdjustVerticalSplittersRecursive(child, leftWidth);
    }
    wxLogDebug("ApplyManual2080Layout: Adjusted %d vertical splitters in alternative approach", adjustedCount);

    // Strategy 3: Adjust bottom splitters
    FindAndAdjustBottomSplitters(container, bottomHeight);

    // Strategy 4: Final fallback - adjust all splitters recursively
    wxLogDebug("ApplyManual2080Layout: Applying final fallback adjustment");
    FindAndAdjustSplitters(container, leftWidth, bottomHeight);

    // Force multiple layout updates to ensure changes take effect
    container->Layout();
    container->Refresh();

    // Additional layout passes
    for (int i = 0; i < 3; ++i) {
        container->Layout();
        wxMilliSleep(10); // Small delay to allow layout to settle
    }
    container->Refresh();

    // Final verification
    wxSize newContainerSize = container->GetSize();
    wxLogDebug("ApplyManual2080Layout: Final container size: %dx%d", newContainerSize.x, newContainerSize.y);

    wxLogDebug("ApplyManual2080Layout: Manual layout application completed");
}

void FlatFrameDocking::ApplyDynamicLayoutAdjustmentOptimized(ads::DockContainerWidget* container) {
    if (!container) {
        return;
    }

    // Get current container size
    wxSize containerSize = container->GetSize();

    // Skip if container is too small
    if (containerSize.x <= 200 || containerSize.y <= 150) {
        return;
    }

    // Get layout configuration
    const ads::DockLayoutConfig& config = m_dockManager->getLayoutConfig();

    if (!config.usePercentage) {
        return;
    }

    // Calculate target sizes based on current container dimensions
    int targetLeftWidth = (containerSize.x * config.leftAreaPercent) / 100;
    int targetBottomHeight = (containerSize.y * config.bottomAreaPercent) / 100;

    // Ensure left area minimum width of 200px (cannot be compressed)
    if (targetLeftWidth < 200) {
        targetLeftWidth = 200;
        wxLogDebug("ApplyDynamicLayoutAdjustmentOptimized: Enforcing minimum left width of 200px");
    }

    // Only proceed if we have reasonable target sizes
    if (targetLeftWidth <= 0) {
        return;
    }

    // Single efficient approach: directly adjust the main vertical splitter
    wxSplitterWindow* mainSplitter = FindMainVerticalSplitter(container);
    if (mainSplitter) {
        // Set minimum pane size to enforce 200px minimum width for left panel
        mainSplitter->SetMinimumPaneSize(200);
        wxLogDebug("ApplyDynamicLayoutAdjustmentOptimized: Set minimum pane size to 200px");

        // Update sash position
        mainSplitter->SetSashPosition(targetLeftWidth);
    }

    // Adjust bottom splitters if needed
    if (config.showBottomArea && targetBottomHeight > 0) {
        FindAndAdjustBottomSplitters(container, targetBottomHeight);
    }

    // Single layout update at the end
    container->Layout();

    // Optional: Update visual appearance
    container->Refresh();
}

void FlatFrameDocking::ApplyDynamicLayoutAdjustment(ads::DockContainerWidget* container) {
    wxLogDebug("ApplyDynamicLayoutAdjustment: Starting dynamic layout adjustment");

    if (!container) {
        wxLogDebug("ApplyDynamicLayoutAdjustment: Container is null");
        return;
    }

    // Get current container size
    wxSize containerSize = container->GetSize();
    wxLogDebug("ApplyDynamicLayoutAdjustment: Container size: %dx%d", containerSize.x, containerSize.y);

    // Get layout configuration
    const ads::DockLayoutConfig& config = m_dockManager->getLayoutConfig();

    if (!config.usePercentage) {
        wxLogDebug("ApplyDynamicLayoutAdjustment: Not using percentage mode, skipping adjustment");
        return;
    }

    // Calculate target sizes based on current container dimensions and percentage config
    int targetLeftWidth = (containerSize.x * config.leftAreaPercent) / 100;
    int targetRightWidth = (containerSize.x * config.rightAreaPercent) / 100;
    int targetTopHeight = (containerSize.y * config.topAreaPercent) / 100;
    int targetBottomHeight = (containerSize.y * config.bottomAreaPercent) / 100;

    // Ensure left area minimum width of 200px (cannot be compressed)
    if (targetLeftWidth < 200) {
        targetLeftWidth = 200;
        wxLogDebug("ApplyDynamicLayoutAdjustment: Enforcing minimum left width of 200px");
    }

    wxLogDebug("ApplyDynamicLayoutAdjustment: Calculated target sizes (after min width check):");
    wxLogDebug("  - Left: %dpx (%d%%)", targetLeftWidth, config.leftAreaPercent);
    wxLogDebug("  - Right: %dpx (%d%%)", targetRightWidth, config.rightAreaPercent);
    wxLogDebug("  - Top: %dpx (%d%%)", targetTopHeight, config.topAreaPercent);
    wxLogDebug("  - Bottom: %dpx (%d%%)", targetBottomHeight, config.bottomAreaPercent);

    // Apply the calculated sizes to splitters
    bool adjustmentMade = false;

    // Strategy 1: Try to use the container's applyLayoutConfig method first
    container->applyLayoutConfig();
    wxLogDebug("ApplyDynamicLayoutAdjustment: Applied container layout config");

    // Strategy 2: Then apply our manual adjustments as backup
    if (config.showLeftArea && targetLeftWidth > 0) {
        wxSplitterWindow* mainSplitter = FindMainVerticalSplitter(container);
        if (mainSplitter) {
            // Set minimum pane size to enforce 200px minimum width for left panel
            mainSplitter->SetMinimumPaneSize(200);
            wxLogDebug("ApplyDynamicLayoutAdjustment: Set minimum pane size to 200px");

            wxLogDebug("ApplyDynamicLayoutAdjustment: Adjusting main vertical splitter to %d", targetLeftWidth);
            mainSplitter->SetSashPosition(targetLeftWidth);
            mainSplitter->UpdateSize();
            adjustmentMade = true;
        }
    }

    // Strategy 3: Adjust all vertical splitters recursively
    int verticalAdjusted = 0;
    wxWindowList& containerChildren = container->GetChildren();
    for (wxWindowList::iterator it = containerChildren.begin(); it != containerChildren.end(); ++it) {
        wxWindow* child = *it;
        verticalAdjusted += AdjustVerticalSplittersRecursive(child, targetLeftWidth);
    }
    if (verticalAdjusted > 0) {
        wxLogDebug("ApplyDynamicLayoutAdjustment: Adjusted %d vertical splitters", verticalAdjusted);
        adjustmentMade = true;
    }

    // Strategy 4: Adjust bottom splitters
    if (config.showBottomArea && targetBottomHeight > 0) {
        FindAndAdjustBottomSplitters(container, targetBottomHeight);
        wxLogDebug("ApplyDynamicLayoutAdjustment: Adjusted bottom splitters");
        adjustmentMade = true;
    }

    // Force layout updates if any adjustments were made
    if (adjustmentMade) {
        wxLogDebug("ApplyDynamicLayoutAdjustment: Adjustments made, forcing layout updates");

        // Multiple layout passes to ensure everything settles
        for (int i = 0; i < 3; ++i) {
            container->Layout();
            wxMilliSleep(5); // Brief delay between layout passes
        }
        container->Refresh();

        wxLogDebug("ApplyDynamicLayoutAdjustment: Layout updates completed");
    } else {
        wxLogDebug("ApplyDynamicLayoutAdjustment: No adjustments needed");
    }

    // Log final result
    wxSize finalSize = container->GetSize();
    wxLogDebug("ApplyDynamicLayoutAdjustment: Final container size: %dx%d", finalSize.x, finalSize.y);

    wxLogDebug("ApplyDynamicLayoutAdjustment: Dynamic layout adjustment completed");
}

int FlatFrameDocking::AdjustVerticalSplittersRecursive(wxWindow* window, int targetLeftWidth) {
    if (!window) return 0;

    int adjustedCount = 0;

    // Check if this is a vertical splitter
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(window);
    if (splitter && splitter->IsSplit() && splitter->GetSplitMode() == wxSPLIT_VERTICAL) {
        // Set minimum pane size to enforce 200px minimum width for left panel
        splitter->SetMinimumPaneSize(200);
        wxLogDebug("AdjustVerticalSplittersRecursive: Set minimum pane size to 200px");

        wxLogDebug("AdjustVerticalSplittersRecursive: Adjusting vertical splitter to %d", targetLeftWidth);
        splitter->SetSashPosition(targetLeftWidth);
        splitter->UpdateSize();
        adjustedCount++;
    }

    // Recursively check all children
    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        adjustedCount += AdjustVerticalSplittersRecursive(child, targetLeftWidth);
    }

    return adjustedCount;
}

wxSplitterWindow* FlatFrameDocking::FindMainVerticalSplitter(wxWindow* window) {
    if (!window) return nullptr;

    // Check if this is a vertical splitter
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(window);
    if (splitter && splitter->IsSplit() && splitter->GetSplitMode() == wxSPLIT_VERTICAL) {
        wxLogDebug("FindMainVerticalSplitter: Found vertical splitter");

        // Check if this looks like the main left/center splitter
        wxWindow* win1 = splitter->GetWindow1();
        wxWindow* win2 = splitter->GetWindow2();

        // The main splitter should have dock areas on both sides
        // This is a heuristic - we assume the splitter with dock widgets is the main one
        if (win1 && win2) {
            wxLogDebug("FindMainVerticalSplitter: Returning vertical splitter as main");
            return splitter;
        }
    }

    // Recursively check all children
    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        wxSplitterWindow* found = FindMainVerticalSplitter(child);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

void FlatFrameDocking::FindAndAdjustBottomSplitters(wxWindow* window, int targetBottomHeight) {
    if (!window) return;

    // Check if this is a horizontal splitter (likely for bottom panels)
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(window);
    if (splitter && splitter->IsSplit() && splitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
        wxLogDebug("FindAndAdjustBottomSplitters: Found horizontal splitter");

        wxWindow* win1 = splitter->GetWindow1();
        wxWindow* win2 = splitter->GetWindow2();

        // Check if this contains bottom panels by looking for our known widgets
        bool containsBottomPanels = ContainsBottomPanels(splitter);
        if (containsBottomPanels) {
            wxLogDebug("FindAndAdjustBottomSplitters: Adjusting bottom splitter to height %d", targetBottomHeight);
            int sashPos = splitter->GetSize().GetHeight() - targetBottomHeight;
            splitter->SetSashPosition(sashPos);
            splitter->UpdateSize();
        }
    }

    // Recursively check all children
    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        FindAndAdjustBottomSplitters(child, targetBottomHeight);
    }
}

bool FlatFrameDocking::ContainsBottomPanels(wxSplitterWindow* splitter) {
    if (!splitter) return false;

    wxWindow* win1 = splitter->GetWindow1();
    wxWindow* win2 = splitter->GetWindow2();

    // Check if either window contains our message or performance panels
    return ContainsWidgetRecursive(win1, m_messageDock) ||
           ContainsWidgetRecursive(win2, m_messageDock) ||
           ContainsWidgetRecursive(win1, m_performanceDock) ||
           ContainsWidgetRecursive(win2, m_performanceDock);
}

bool FlatFrameDocking::ContainsWidgetRecursive(wxWindow* container, ads::DockWidget* targetWidget) {
    if (!container || !targetWidget) return false;

    // Check if this container directly contains our target widget
    if (container == targetWidget->widget()) {
        return true;
    }

    // Recursively check children
    wxWindowList& children = container->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        if (ContainsWidgetRecursive(child, targetWidget)) {
            return true;
        }
    }

    return false;
}

void FlatFrameDocking::FindAndAdjustSplitters(wxWindow* window, int targetLeftWidth, int targetBottomHeight) {
    if (!window) return;

    // Check if this is a splitter
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(window);
    if (splitter && splitter->IsSplit()) {
        wxLogDebug("FindAndAdjustSplitters: Found splitter, orientation: %s",
                   splitter->GetSplitMode() == wxSPLIT_VERTICAL ? "vertical" : "horizontal");

        // Determine splitter type by checking its children
        wxWindow* win1 = splitter->GetWindow1();
        wxWindow* win2 = splitter->GetWindow2();

        if (splitter->GetSplitMode() == wxSPLIT_VERTICAL) {
            // Vertical splitter (left/right) - set minimum pane size
            splitter->SetMinimumPaneSize(200);
            wxLogDebug("FindAndAdjustSplitters: Set minimum pane size to 200px for vertical splitter");
            wxLogDebug("FindAndAdjustSplitters: Adjusting vertical splitter to %d", targetLeftWidth);
            splitter->SetSashPosition(targetLeftWidth);
        } else {
            // Horizontal splitter (top/bottom)
            wxLogDebug("FindAndAdjustSplitters: Adjusting horizontal splitter to %d", targetBottomHeight);
            splitter->SetSashPosition(splitter->GetSize().GetHeight() - targetBottomHeight);
        }
    }

    // Recursively check all children
    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxWindow* child = *it;
        this->FindAndAdjustSplitters(child, targetLeftWidth, targetBottomHeight);
    }
}