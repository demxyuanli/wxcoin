# Qt-Advanced-Docking-System Implementation

This project includes a complete implementation of the Qt-Advanced-Docking-System functionality for wxWidgets applications. The docking system provides a modern, flexible dock widget system similar to Visual Studio, Qt Creator, and other professional IDEs.

## Features

- **Dockable Widgets**: Create widgets that can be docked to any side of the main window
- **Tabbed Interface**: Multiple widgets can be tabbed together in the same area
- **Floating Windows**: Dock widgets can be undocked to create floating windows
- **Drag and Drop**: Full drag and drop support for rearranging dock widgets
- **Splitter Support**: Resizable dock areas with splitter handles
- **State Persistence**: Save and restore complete dock layouts
- **Customizable Behavior**: Configure close buttons, tab behavior, and more
- **Visual Feedback**: Drop indicators show where widgets will be docked

## Architecture

The docking system consists of several key components:

### Core Components

1. **DockManager** (`include/docking/DockManager.h`)
   - Central manager for all docking operations
   - Handles widget registration and layout management
   - Provides configuration options and state persistence

2. **DockWidget** (`include/docking/DockWidget.h`)
   - Individual dockable widget container
   - Manages widget state, features, and visibility
   - Supports custom title bars and close handlers

3. **DockArea** (`include/docking/DockArea.h`)
   - Container for multiple dock widgets in tabs
   - Manages tab bar and title bar
   - Handles widget switching and closing

4. **DockContainerWidget** (`include/docking/DockContainerWidget.h`)
   - Main container that manages dock area layout
   - Handles splitter-based layouts
   - Manages dock area positioning

5. **FloatingDockContainer** (`include/docking/FloatingDockContainer.h`)
   - Floating window container for undocked widgets
   - Supports native or custom title bars
   - Handles dragging and docking operations

6. **DockOverlay** (`include/docking/DockOverlay.h`)
   - Visual feedback during drag and drop
   - Shows drop indicators for docking positions

## Usage

### Basic Integration

Replace the existing ModernDockAdapter in FlatFrame with the docking system:

```cpp
#include "docking/DockManager.h"
#include "docking/DockWidget.h"

void FlatFrame::createPanels() {
    wxBoxSizer* mainSizer = GetMainSizer();
    mainSizer->Add(m_ribbon, 0, wxEXPAND | wxALL, 1);
    
    // Create dock manager
    auto* dockManager = new ads::DockManager(this);
    mainSizer->Add(dockManager->containerWidget(), 1, wxEXPAND | wxALL, 2);
    
    // Create dock widgets
    auto* treeWidget = new ads::DockWidget("Object Tree");
    treeWidget->setWidget(m_objectTreePanel);
    
    auto* propWidget = new ads::DockWidget("Properties");
    propWidget->setWidget(m_propertyPanel);
    
    auto* canvasWidget = new ads::DockWidget("3D View");
    canvasWidget->setWidget(m_canvas);
    canvasWidget->setFeature(ads::DockWidgetClosable, false);
    
    // Add to dock manager
    dockManager->addDockWidget(ads::LeftDockWidgetArea, treeWidget);
    dockManager->addDockWidget(ads::LeftDockWidgetArea, propWidget, 
                              treeWidget->dockAreaWidget());
    dockManager->addDockWidget(ads::CenterDockWidgetArea, canvasWidget);
}
```

### Using the Integration Helper

The `DockingIntegration` class provides helper methods for common setups:

```cpp
#include "docking/DockingIntegration.h"

// Create standard CAD layout
ads::DockManager* dockManager = ads::DockingIntegration::CreateStandardCADLayout(
    this,           // parent window
    m_canvas,       // main 3D view
    m_objectTree,   // object tree panel
    m_properties,   // properties panel
    m_messageOutput,// message output
    m_performancePanel // optional performance panel
);

// Add example widgets for testing
ads::DockingIntegration::CreateExampleDockWidgets(dockManager);

// Setup view menu
ads::DockingIntegration::SetupViewMenu(viewMenu, dockManager);
```

### Creating Custom Dock Widgets

```cpp
// Create a custom panel
wxPanel* myPanel = new wxPanel(dockManager->containerWidget());
// ... setup panel content ...

// Create dock widget
ads::DockWidget* myWidget = new ads::DockWidget("My Tool");
myWidget->setWidget(myPanel);
myWidget->setObjectName("MyTool"); // For state persistence
myWidget->setIcon(myIcon);         // Optional icon

// Configure features
myWidget->setFeature(ads::DockWidgetClosable, true);
myWidget->setFeature(ads::DockWidgetFloatable, true);
myWidget->setFeature(ads::DockWidgetMovable, true);

// Add to dock manager
dockManager->addDockWidget(ads::RightDockWidgetArea, myWidget);
```

### Saving and Restoring Layouts

```cpp
// Save layout
wxString layoutData;
dockManager->saveState(layoutData);
// Save layoutData to config file or preferences

// Restore layout
wxString savedLayout = LoadFromConfig();
dockManager->restoreState(savedLayout);
```

### Configuration Options

```cpp
// Configure dock manager behavior
dockManager->setConfigFlag(ads::OpaqueSplitterResize, true);
dockManager->setConfigFlag(ads::AlwaysShowTabs, false);
dockManager->setConfigFlag(ads::AllTabsHaveCloseButton, true);
dockManager->setConfigFlag(ads::DockAreaHasCloseButton, true);
dockManager->setConfigFlag(ads::FocusHighlighting, true);
```

## Example Application

A complete example application is provided in `src/docking/DockingExample.cpp`. To build and run:

```bash
# The example is included in the docking library
# It demonstrates all features of the docking system
```

The example shows:
- Creating various types of dock widgets
- Drag and drop functionality
- Floating windows
- Tab management
- Layout save/restore
- Custom widget creation

## Building

The docking system is built as part of the main project:

```bash
mkdir build
cd build
cmake ..
make
```

The docking library is automatically included when building the main application.

## Integration with Existing Code

To integrate with the existing FlatFrame:

1. Include the docking headers:
   ```cpp
   #include "docking/DockManager.h"
   #include "docking/DockWidget.h"
   #include "docking/DockingIntegration.h"
   ```

2. Replace ModernDockAdapter with DockManager in FlatFrameInit.cpp

3. Update the UI module's CMakeLists.txt to link with the docking library:
   ```cmake
   target_link_libraries(CADUI PRIVATE
       # ... existing libraries ...
       docking
   )
   ```

## Advanced Features

### Custom Close Handlers

```cpp
widget->setCloseHandler([]() {
    if (wxMessageBox("Save changes?", "Close", wxYES_NO) == wxYES) {
        // Save changes
        return true; // Allow close
    }
    return false; // Cancel close
});
```

### Programmatic Layout Control

```cpp
// Float a widget
widget->setFloating();

// Make widget the current tab
widget->setAsCurrentTab();

// Close widget
widget->closeDockWidget();

// Get all widgets in an area
DockArea* area = widget->dockAreaWidget();
std::vector<DockWidget*> widgets = area->dockWidgets();
```

### Event Handling

```cpp
// Connect to dock widget events
widget->Bind(ads::DockWidget::EVT_DOCK_WIDGET_CLOSING, [](wxCommandEvent& event) {
    // Handle widget closing
});

widget->Bind(ads::DockWidget::EVT_DOCK_WIDGET_VISIBILITY_CHANGED, [](wxCommandEvent& event) {
    // Handle visibility change
});
```

## License

This implementation is based on the Qt-Advanced-Docking-System concept but is a complete rewrite for wxWidgets. It can be used under the same license as your project.
