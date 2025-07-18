# Advanced Edge Settings Feature

## Overview

The Advanced Edge Settings feature provides comprehensive control over edge display parameters for different object states (global, selected, and hover) in the CAD application. This feature replaces the previous simple edge settings with a more sophisticated system that allows users to configure edge appearance based on object interaction states.

## Features

### 1. Multi-State Edge Configuration
- **Global Objects**: Default edge settings for all objects
- **Selected Objects**: Edge settings applied when objects are selected
- **Hover Objects**: Edge settings applied when mouse hovers over objects

### 2. Configurable Parameters
Each state supports the following parameters:
- **Show Edges**: Enable/disable edge display
- **Edge Width**: Line thickness (0.1 to 5.0)
- **Edge Color**: RGB color selection with enable/disable option
- **Edge Style**: Solid, Dashed, Dotted, Dash-Dot
- **Edge Opacity**: Transparency level (10% to 100%)

### 3. Configuration Persistence
- Settings are automatically saved to `edge_settings.ini` in the root directory
- Configuration file is loaded on application startup
- If no configuration file exists, default settings are used
- Settings persist between application sessions

## Implementation Details

### Core Components

#### 1. EdgeSettingsConfig
- **Location**: `include/config/EdgeSettingsConfig.h`, `src/config/EdgeSettingsConfig.cpp`
- **Purpose**: Singleton configuration manager for edge settings
- **Features**:
  - Load/save settings from/to INI file
  - Provide settings for different object states
  - Notification system for settings changes
  - Default value management

#### 2. EdgeSettingsDialog
- **Location**: `include/EdgeSettingsDialog.h`, `src/ui/EdgeSettingsDialog.cpp`
- **Purpose**: Multi-page dialog for configuring edge settings
- **Features**:
  - Tabbed interface for different object states
  - Real-time preview of color changes
  - Apply, Save, Reset, and OK/Cancel buttons
  - Integration with EdgeSettingsConfig

#### 3. EdgeSettingsListener
- **Location**: `include/EdgeSettingsListener.h`, `src/commands/EdgeSettingsListener.cpp`
- **Purpose**: Command listener for edge settings dialog
- **Features**:
  - Integration with command system
  - Feedback on settings application
  - Support for selected vs. all objects

#### 4. OCCMeshConverter Integration
- **Location**: `src/opencascade/OCCMeshConverter.cpp`
- **Purpose**: Apply edge settings during mesh conversion
- **Features**:
  - Dynamic edge color application based on object state
  - Integration with EdgeSettingsConfig
  - Support for edge color enable/disable

### Configuration File Format

The edge settings are stored in `edge_settings.ini` in the root directory:

```ini
[Global]
ShowEdges=true
EdgeWidth=1.0
EdgeColor=0.0,0.0,0.0
EdgeColorEnabled=true
EdgeStyle=0
EdgeOpacity=1.0

[Selected]
ShowEdges=true
EdgeWidth=2.0
EdgeColor=1.0,0.0,0.0
EdgeColorEnabled=true
EdgeStyle=0
EdgeOpacity=1.0

[Hover]
ShowEdges=true
EdgeWidth=1.5
EdgeColor=0.0,1.0,0.0
EdgeColorEnabled=true
EdgeStyle=1
EdgeOpacity=0.8
```

### Usage Examples

#### 1. Basic Usage
```cpp
// Get edge settings for different states
EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
EdgeSettings globalSettings = config.getGlobalSettings();
EdgeSettings selectedSettings = config.getSelectedSettings();
EdgeSettings hoverSettings = config.getHoverSettings();

// Apply settings to geometry
geometry->setShowEdges(globalSettings.showEdges);
geometry->setEdgeWidth(globalSettings.edgeWidth);
geometry->setEdgeColor(globalSettings.edgeColor);
```

#### 2. Dynamic State Application
```cpp
// In OCCMeshConverter::createCoinNode
EdgeSettingsConfig& edgeConfig = EdgeSettingsConfig::getInstance();
EdgeSettings edgeSettings = selected ? 
    edgeConfig.getSelectedSettings() : 
    edgeConfig.getGlobalSettings();

if (edgeSettings.showEdges) {
    // Apply edge settings based on object state
    if (edgeSettings.edgeColorEnabled) {
        edgeMaterial->diffuseColor.setValue(
            edgeSettings.edgeColor.Red(),
            edgeSettings.edgeColor.Green(),
            edgeSettings.edgeColor.Blue()
        );
    }
}
```

#### 3. Settings Change Notification
```cpp
// Register callback for settings changes
EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
config.addSettingsChangedCallback([]() {
    // Refresh all geometries when settings change
    refreshAllGeometries();
});
```

## UI Integration

### Menu Integration
The edge settings dialog is accessible through:
- **Menu**: View → Edge Settings
- **Command**: `cmd::CommandType::EdgeSettings`

### Dialog Features
- **Global Objects Tab**: Configure default edge settings
- **Selected Objects Tab**: Configure edge settings for selected objects
- **Hover Objects Tab**: Configure edge settings for hover state
- **Apply Button**: Apply settings to geometries immediately (not saved to file)
- **Save Config Button**: Save settings to configuration file (not applied to geometries)
- **Reset Button**: Reset to default values
- **OK Button**: Apply settings to geometries AND save to file
- **Cancel Button**: Discard changes and close dialog

## Technical Architecture

### Data Flow
1. **User Input**: User modifies settings in EdgeSettingsDialog
2. **Apply Button**: Settings are applied to geometries immediately (not saved to file)
3. **OK Button**: Settings are applied to geometries AND saved to file
4. **Save Config Button**: Settings are saved to file without applying
5. **Mesh Conversion**: OCCMeshConverter applies settings during mesh creation
6. **Rendering**: Coin3D renders edges with applied settings
7. **Persistence**: Settings are saved to INI file in root directory

### State Management
- **Global State**: Applied to all objects by default
- **Selected State**: Applied when objects are selected (future enhancement)
- **Hover State**: Applied when mouse hovers over objects (future enhancement)

### Performance Considerations
- Settings are cached in memory for fast access
- Configuration file is only read/written when necessary
- Edge settings are applied during mesh generation, not at render time
- Minimal overhead for settings queries

## Future Enhancements

### Planned Features
1. **Real-time State Detection**: Automatic application of hover/selected settings
2. **Per-Object Settings**: Individual edge settings for specific objects
3. **Edge Style Implementation**: Support for dashed, dotted, and dash-dot styles
4. **Edge Opacity Support**: Transparency implementation in Coin3D
5. **Edge Width Support**: Line thickness implementation in Coin3D

### Integration Points
1. **Selection System**: Integration with object selection for selected state
2. **Mouse Interaction**: Integration with mouse hover detection
3. **Object Tree**: Integration with object tree for per-object settings
4. **Undo/Redo**: Support for settings changes in undo/redo system

## Troubleshooting

### Common Issues
1. **Settings Not Applied**: Check if EdgeSettingsConfig is properly initialized
2. **Color Not Visible**: Verify edge color is enabled and not black
3. **Settings Not Saved**: Check file permissions for config directory
4. **Performance Issues**: Ensure settings are not queried too frequently

### Debug Information
- All edge settings operations are logged with appropriate log levels
- Use `LOG_INF_S()` for successful operations
- Use `LOG_WRN_S()` for warnings
- Use `LOG_ERR_S()` for errors

## Dependencies

### Required Libraries
- **OpenCASCADE**: For geometry and mesh operations
- **Coin3D**: For 3D rendering and material properties
- **wxWidgets**: For UI components and dialogs
- **Standard C++**: For file I/O and data structures

### Internal Dependencies
- **RenderingConfig**: For integration with existing rendering system
- **Command System**: For command-based access to settings
- **Logger**: For debug and error logging
- **SceneManager**: For scene integration and refresh management 