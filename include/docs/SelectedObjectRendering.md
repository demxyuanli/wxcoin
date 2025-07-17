# Selected Object Rendering System

## Overview

The selected object rendering system allows users to apply rendering settings (materials, textures, transparency, etc.) specifically to selected geometry objects. If no objects are selected, the settings are applied to all geometry objects in the scene.

## Key Components

### 1. RenderingConfig Enhancements

Added new methods to `RenderingConfig` class to support selected object rendering:

#### Selected Object Settings Methods
- `applyMaterialSettingsToSelected()` - Apply material settings to selected objects
- `applyTextureSettingsToSelected()` - Apply texture settings to selected objects
- `applyBlendSettingsToSelected()` - Apply blend settings to selected objects
- `applyShadingSettingsToSelected()` - Apply shading settings to selected objects
- `applyDisplaySettingsToSelected()` - Apply display settings to selected objects

#### Individual Property Setters for Selected Objects
- `setSelectedMaterialDiffuseColor()` - Set diffuse color for selected objects
- `setSelectedMaterialTransparency()` - Set transparency for selected objects
- `setSelectedTextureEnabled()` - Enable/disable texture for selected objects
- `setSelectedTextureMode()` - Set texture mode for selected objects
- `setSelectedBlendMode()` - Set blend mode for selected objects
- And many more individual property setters...

#### Utility Methods
- `hasSelectedObjects()` - Check if any objects are selected
- `applyMaterialPresetToSelected()` - Apply material preset to selected objects

### 2. SceneManager Callback Enhancement

Modified `SceneManager::initializeRenderingConfigCallback()` to handle selected object rendering:

```cpp
void SceneManager::initializeRenderingConfigCallback()
{
    RenderingConfig& config = RenderingConfig::getInstance();
    config.registerSettingsChangedCallback([this]() {
        if (m_canvas && m_canvas->getOCCViewer()) {
            OCCViewer* viewer = m_canvas->getOCCViewer();
            auto selectedGeometries = viewer->getSelectedGeometries();
            
            if (!selectedGeometries.empty()) {
                // Update only selected geometries
                for (auto& geometry : selectedGeometries) {
                    geometry->updateFromRenderingConfig();
                }
            } else {
                // Update all geometries if none are selected
                auto allGeometries = viewer->getAllGeometry();
                for (auto& geometry : allGeometries) {
                    geometry->updateFromRenderingConfig();
                }
            }
            
            // Force refresh
            if (m_canvas->getRefreshManager()) {
                m_canvas->getRefreshManager()->requestRefresh(
                    ViewRefreshManager::RefreshReason::RENDERING_CHANGED, true);
            }
            m_canvas->Refresh(true);
            m_canvas->Update();
        }
    });
}
```

### 3. Command Listeners Update

Updated all texture mode command listeners to use selected object methods:

#### TextureModeDecalListener
```cpp
// Before: Applied to all objects
config.setTextureEnabled(true);
config.setTextureMode(RenderingConfig::TextureMode::Decal);

// After: Applied to selected objects
config.setSelectedTextureEnabled(true);
config.setSelectedTextureMode(RenderingConfig::TextureMode::Decal);
```

#### TextureModeModulateListener
#### TextureModeReplaceListener  
#### TextureModeBlendListener

All follow the same pattern of using `setSelected*` methods instead of global methods.

### 4. Test Button

Added a test button in the Tools page to verify selected object rendering:

- **Button ID**: `ID_TEST_SELECTED_RENDERING`
- **Location**: Tools page, "Test Selected" button
- **Function**: Applies a distinctive red material with green texture to selected objects

## Usage

### 1. Select Objects
- Use the object tree panel or click on objects in the 3D view to select them
- Multiple objects can be selected by holding Ctrl/Cmd while clicking

### 2. Apply Rendering Settings
- Use any of the texture mode buttons (Decal, Modulate, Replace, Blend)
- Use the "Set Transparency" command
- Use the "Test Selected" button to apply a test material

### 3. Behavior
- **With Selection**: Settings are applied only to selected objects
- **Without Selection**: Settings are applied to all objects in the scene

## Technical Implementation

### Selection Detection
The system uses `OCCViewer::getSelectedGeometries()` to detect selected objects:

```cpp
auto selectedGeometries = m_occViewer->getSelectedGeometries();
if (!selectedGeometries.empty()) {
    // Apply to selected objects only
} else {
    // Apply to all objects
}
```

### Notification System
The existing `RenderingConfig` notification system is leveraged:
1. Settings are updated via `setSelected*` methods
2. `notifySettingsChanged()` is called
3. SceneManager callback is triggered
4. Selected objects (or all objects if none selected) are updated
5. Canvas is refreshed

### Coin3D Integration
The system works with the existing Coin3D rendering pipeline:
- `OCCGeometry::updateFromRenderingConfig()` rebuilds Coin3D representation
- Coin3D nodes are touched to force cache invalidation
- Scene graph is updated for immediate visual feedback

## Benefits

1. **Precise Control**: Apply different materials to different objects
2. **Non-Destructive**: Global settings remain unchanged for unselected objects
3. **Intuitive**: No selection = affect all objects (backward compatibility)
4. **Real-time**: Immediate visual feedback when settings are applied
5. **Consistent**: Uses existing command system and notification mechanisms

## Future Enhancements

1. **Selection Memory**: Remember individual object settings when selection changes
2. **Material Presets**: Apply different material presets to different objects
3. **Batch Operations**: Apply settings to objects by type, name, or other criteria
4. **Undo/Redo**: Support for undoing rendering changes on selected objects
5. **Visual Feedback**: Highlight selected objects with different visual indicators 