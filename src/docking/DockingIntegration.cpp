#include "docking/DockingIntegration.h"
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>

namespace ads {


DockManager* DockingIntegration::CreateStandardCADLayout(
    wxWindow* parent,
    wxWindow* canvas,
    wxWindow* objectTree,
    wxWindow* properties,
    wxWindow* messageOutput,
    wxWindow* performancePanel)
{
    // Create dock manager
    DockManager* dockManager = new DockManager(parent);
    
    // Configure features for CAD application
    dockManager->setConfigFlag(OpaqueSplitterResize, true);
    dockManager->setConfigFlag(FocusHighlighting, true);
    dockManager->setConfigFlag(DockAreaHasCloseButton, true);
    dockManager->setConfigFlag(AllTabsHaveCloseButton, true);
    dockManager->setConfigFlag(AlwaysShowTabs, false);
    
    // Create dock widgets
    DockWidget* canvasWidget = new DockWidget("3D View", dockManager->containerWidget());
    canvasWidget->setWidget(canvas);
    canvasWidget->setFeature(DockWidgetClosable, false); // Main view can't be closed
    canvasWidget->setObjectName("3DView");
    
    DockWidget* treeWidget = new DockWidget("Object Tree", dockManager->containerWidget());
    treeWidget->setWidget(objectTree);
    treeWidget->setObjectName("ObjectTree");
    
    DockWidget* propWidget = new DockWidget("Properties", dockManager->containerWidget());
    propWidget->setWidget(properties);
    propWidget->setObjectName("Properties");
    
    DockWidget* messageWidget = new DockWidget("Messages", dockManager->containerWidget());
    messageWidget->setWidget(messageOutput);
    messageWidget->setObjectName("Messages");
    
    // Add to dock manager
    dockManager->addDockWidget(CenterDockWidgetArea, canvasWidget);
    
    // Add left side panels
    DockArea* leftArea = dockManager->addDockWidget(LeftDockWidgetArea, treeWidget);
    dockManager->addDockWidget(BottomDockWidgetArea, propWidget, leftArea);
    
    // Add bottom panel
    DockArea* bottomArea = dockManager->addDockWidget(BottomDockWidgetArea, messageWidget);
    
    // Add performance panel if provided
    if (performancePanel) {
        DockWidget* perfWidget = new DockWidget("Performance", dockManager->containerWidget());
        perfWidget->setWidget(performancePanel);
        perfWidget->setObjectName("Performance");
        dockManager->addDockWidgetTabToArea(perfWidget, bottomArea);
    }
    
    return dockManager;
}

void DockingIntegration::CreateExampleDockWidgets(DockManager* dockManager)
{
    if (!dockManager) {
        return;
    }
    
    // Create example tool windows
    
    // Example 1: Tool palette
    wxPanel* toolPanel = new wxPanel(dockManager->containerWidget());
    wxBoxSizer* toolSizer = new wxBoxSizer(wxVERTICAL);
    
    wxButton* selectBtn = new wxButton(toolPanel, wxID_ANY, "Select");
    wxButton* moveBtn = new wxButton(toolPanel, wxID_ANY, "Move");
    wxButton* rotateBtn = new wxButton(toolPanel, wxID_ANY, "Rotate");
    wxButton* scaleBtn = new wxButton(toolPanel, wxID_ANY, "Scale");
    
    toolSizer->Add(selectBtn, 0, wxEXPAND | wxALL, 2);
    toolSizer->Add(moveBtn, 0, wxEXPAND | wxALL, 2);
    toolSizer->Add(rotateBtn, 0, wxEXPAND | wxALL, 2);
    toolSizer->Add(scaleBtn, 0, wxEXPAND | wxALL, 2);
    toolSizer->AddStretchSpacer();
    
    toolPanel->SetSizer(toolSizer);
    
    DockWidget* toolWidget = new DockWidget("Tools", dockManager->containerWidget());
    toolWidget->setWidget(toolPanel);
    toolWidget->setObjectName("ToolPalette");
    dockManager->addDockWidget(LeftDockWidgetArea, toolWidget);
    
    // Example 2: Layers panel
    wxListCtrl* layersList = new wxListCtrl(dockManager->containerWidget(), wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    layersList->AppendColumn("Layer", wxLIST_FORMAT_LEFT, 150);
    layersList->AppendColumn("Visible", wxLIST_FORMAT_CENTER, 60);
    
    // Add some example layers
    long item = layersList->InsertItem(0, "Default");
    layersList->SetItem(item, 1, "Yes");
    
    item = layersList->InsertItem(1, "Annotations");
    layersList->SetItem(item, 1, "Yes");
    
    item = layersList->InsertItem(2, "Construction");
    layersList->SetItem(item, 1, "No");
    
    DockWidget* layersWidget = new DockWidget("Layers", dockManager->containerWidget());
    layersWidget->setWidget(layersList);
    layersWidget->setObjectName("Layers");
    dockManager->addDockWidget(RightDockWidgetArea, layersWidget);
    
    // Example 3: Console output
    wxTextCtrl* console = new wxTextCtrl(dockManager->containerWidget(), wxID_ANY,
        "Console output...\n", wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    console->SetBackgroundColour(wxColour(30, 30, 30));
    console->SetForegroundColour(wxColour(200, 200, 200));
    
    DockWidget* consoleWidget = new DockWidget("Console", dockManager->containerWidget());
    consoleWidget->setWidget(console);
    consoleWidget->setObjectName("Console");
    dockManager->addDockWidget(BottomDockWidgetArea, consoleWidget);
    
    // Example 4: Floating calculator
    wxPanel* calcPanel = new wxPanel(dockManager->containerWidget());
    wxTextCtrl* calcDisplay = new wxTextCtrl(calcPanel, wxID_ANY, "0",
        wxDefaultPosition, wxDefaultSize, wxTE_RIGHT | wxTE_READONLY);
    
    DockWidget* calcWidget = new DockWidget("Calculator", dockManager->containerWidget());
    calcWidget->setWidget(calcPanel);
    calcWidget->setObjectName("Calculator");
    
    // Make it floating by default
    dockManager->addDockWidgetFloating(calcWidget);
}

void DockingIntegration::SetupViewMenu(wxMenu* menu, DockManager* dockManager)
{
    if (!menu || !dockManager) {
        return;
    }
    
    // Add separator if menu has items
    if (menu->GetMenuItemCount() > 0) {
        menu->AppendSeparator();
    }
    
    // Add menu items for each dock widget
    std::vector<DockWidget*> widgets = dockManager->dockWidgets();
    
    for (auto* widget : widgets) {
        // Skip widgets that don't have a toggle action
        if (!widget->toggleViewAction()) {
            continue;
        }
        
        // Add to menu
        menu->Append(widget->toggleViewAction());
    }
    
    // Add layout management items
    menu->AppendSeparator();
    menu->Append(wxID_ANY, "Save Layout...");
    menu->Append(wxID_ANY, "Restore Layout...");
    menu->Append(wxID_ANY, "Reset to Default Layout");
}

} // namespace ads
