# Docking System Integration Guide

## Overview

The docking system has been integrated into the main CAD application as a module, providing advanced window management capabilities.

## Integration Components

### 1. FlatFrameDocking Class
- Extends the existing `FlatFrame` class
- Replaces wxAUI with the advanced docking system
- Located in `include/FlatFrameDocking.h` and `src/ui/FlatFrameDocking.cpp`

### 2. Key Features Integrated

#### Dockable Panels
- **3D View (Canvas)**: Main viewport, non-closable
- **Properties Panel**: Object properties, can be auto-hidden
- **Object Tree**: Scene hierarchy, can be auto-hidden
- **Output Window**: Application messages, can be auto-hidden
- **Toolbox**: Tool selection, can be auto-hidden

#### Layout Management
- Save/Load layouts as XML files
- Named perspectives with quick switching
- Auto-save option for perspectives
- Reset to default layout

#### Advanced Features
- **Auto-hide**: Pin/unpin panels to screen edges
- **Floating Windows**: Undock panels to separate windows
- **Tabbed Docking**: Multiple panels in same area
- **Dynamic Splitters**: Resize dock areas
- **Drag Preview**: Visual feedback during dragging

## Usage

### Building with Docking

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release

# Run integration test
./test/docking_integration_test
```

### Menu Integration

The View menu includes new docking options:
- **Save Layout** (Ctrl+L): Save current layout
- **Load Layout** (Ctrl+Shift+L): Load saved layout
- **Reset Layout**: Restore default arrangement
- **Manage Perspectives**: Open perspective manager
- **Toggle Auto-hide** (Ctrl+H): Auto-hide current panel

### Code Integration Example

```cpp
// In your main application
#include "FlatFrameDocking.h"

bool MyApp::OnInit() {
    // Create main frame with docking
    FlatFrameDocking* frame = new FlatFrameDocking(
        "My CAD Application", 
        wxDefaultPosition, 
        wxSize(1200, 800)
    );
    
    frame->Show(true);
    return true;
}
```

### Creating Custom Dock Widgets

```cpp
// Create a custom dockable panel
ads::DockWidget* CreateCustomPanel(ads::DockManager* dockManager) {
    ads::DockWidget* dock = new ads::DockWidget("My Panel", dockManager);
    
    // Create your custom control
    wxPanel* panel = new wxPanel(dock);
    // ... setup panel content ...
    
    dock->setWidget(panel);
    dock->setFeature(ads::DockWidget::DockWidgetClosable, true);
    dock->setFeature(ads::DockWidget::DockWidgetMovable, true);
    dock->setFeature(ads::DockWidget::DockWidgetFloatable, true);
    dock->setFeature(ads::DockWidget::DockWidgetPinnable, true);
    
    return dock;
}
```

## Architecture

### Class Hierarchy
```
FlatFrame (existing)
    ↓
FlatFrameDocking (new)
    - Uses DockManager instead of wxAuiManager
    - Converts existing panels to DockWidgets
    - Adds docking-specific menu items
```

### Dependencies
- `docking`: Core docking library
- `UIFrame`: Main UI framework
- `CADConfig`: Configuration management
- `CADLogger`: Logging system

## Migration from wxAUI

### Key Differences
1. **Panel Management**: Use DockWidget instead of wxAuiPaneInfo
2. **Layout Persistence**: XML format instead of wxAUI perspective strings
3. **Events**: New event system for dock widgets
4. **Features**: More granular control over widget behavior

### Migration Steps
1. Replace wxAuiManager with DockManager
2. Convert panels to DockWidgets
3. Update menu handlers for new features
4. Migrate saved layouts (if needed)

## Testing

### Unit Tests
Run the standalone docking tests:
```bash
./test/docking_test
```

### Integration Tests
Run the integration test with the main framework:
```bash
./test/docking_integration_test
```

### Manual Testing Checklist
- [ ] Dock/undock panels by dragging
- [ ] Create floating windows
- [ ] Test auto-hide (pin/unpin)
- [ ] Save and load layouts
- [ ] Switch between perspectives
- [ ] Resize dock areas
- [ ] Close and restore panels
- [ ] Test with multiple monitors

## Troubleshooting

### Common Issues

1. **Panels not visible**: Check if they're minimized or auto-hidden
2. **Layout not saving**: Ensure write permissions for XML files
3. **Drag not working**: Verify DockWidget movable feature is enabled
4. **Auto-hide not working**: Check if pinnable feature is enabled

### Debug Mode
Enable debug output:
```cpp
dockManager->setConfigFlag(DockManager::DebugMessages, true);
```

## Future Enhancements

- Custom themes for dock widgets
- Keyboard shortcuts for dock operations
- Dock widget templates
- Integration with undo/redo system
- Multi-window support
