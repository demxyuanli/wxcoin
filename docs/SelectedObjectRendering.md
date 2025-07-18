# Selected Object Rendering System

## Overview

The selected object rendering system allows users to apply rendering settings (texture modes, transparency, material properties, etc.) to either selected objects or all objects in the scene. When objects are selected, the settings are applied only to those objects. When no objects are selected, the settings are applied to all objects.

## Key Features

### 1. Automatic Selection Detection
- The system automatically detects whether any objects are selected
- If objects are selected, settings are applied only to selected objects
- If no objects are selected, settings are applied to all objects

### 2. Texture Mode Commands
The following texture mode commands now support selected object rendering:

#### TextureModeDecalListener
- Applies Decal texture mode to selected objects or all objects
- Uses bright red texture color with green base material for contrast
- Full texture intensity for maximum visibility

#### TextureModeModulateListener
- Applies Modulate texture mode to selected objects or all objects
- Uses bright yellow texture color with purple base material
- Moderate texture intensity for modulation effect

#### TextureModeReplaceListener
- Applies Replace texture mode to selected objects or all objects
- Uses bright cyan texture color with reddish base material
- Full texture intensity to completely replace base color

#### TextureModeBlendListener
- Applies Blend texture mode to selected objects or all objects
- Uses bright magenta texture color with green base material
- Moderate texture intensity with some transparency for blend effect

### 3. Transparency Command
#### SetTransparencyListener
- Applies transparency settings to selected objects or all objects
- Shows transparency dialog for selected geometries or all geometries
- Supports individual transparency control for each object

### 4. RenderingConfig Selected Object Methods
The RenderingConfig class now includes methods for applying settings to selected objects:

#### Material Settings
- `setSelectedMaterialAmbientColor()` - Set ambient color for selected objects
- `setSelectedMaterialDiffuseColor()` - Set diffuse color for selected objects
- `setSelectedMaterialSpecularColor()` - Set specular color for selected objects
- `setSelectedMaterialShininess()` - Set shininess for selected objects
- `setSelectedMaterialTransparency()` - Set transparency for selected objects

#### Texture Settings
- `setSelectedTextureEnabled()` - Enable/disable texture for selected objects
- `setSelectedTextureColor()` - Set texture color for selected objects
- `setSelectedTextureIntensity()` - Set texture intensity for selected objects
- `setSelectedTextureImagePath()` - Set texture image path for selected objects
- `setSelectedTextureMode()` - Set texture mode for selected objects

#### Blend Settings
- `setSelectedBlendMode()` - Set blend mode for selected objects
- `setSelectedDepthTest()` - Enable/disable depth test for selected objects
- `setSelectedDepthWrite()` - Enable/disable depth write for selected objects
- `setSelectedCullFace()` - Enable/disable face culling for selected objects
- `setSelectedAlphaThreshold()` - Set alpha threshold for selected objects

#### Display Settings
- `setSelectedDisplayMode()` - Set display mode for selected objects
- `setSelectedShowEdges()` - Show/hide edges for selected objects
- `setSelectedShowVertices()` - Show/hide vertices for selected objects
- `setSelectedEdgeWidth()` - Set edge width for selected objects
- `setSelectedVertexSize()` - Set vertex size for selected objects
- `setSelectedEdgeColor()` - Set edge color for selected objects
- `setSelectedVertexColor()` - Set vertex color for selected objects

#### Shading Settings
- `setSelectedShadingMode()` - Set shading mode for selected objects
- `setSelectedSmoothNormals()` - Enable/disable smooth normals for selected objects
- `setSelectedWireframeWidth()` - Set wireframe width for selected objects
- `setSelectedPointSize()` - Set point size for selected objects

### 5. Utility Methods
- `hasSelectedObjects()` - Check if any objects are selected
- `getOCCViewerInstance()` - Get OCCViewer instance for selection checking

## Implementation Details

### SceneManager Callback
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
            m_canvas->Refresh(true);
        }
    });
}
```

### Command Listener Logic
Each texture mode listener now includes selection detection:

```cpp
// Check if any objects are selected
auto selectedGeometries = m_viewer->getSelectedGeometries();
bool hasSelection = !selectedGeometries.empty();

if (hasSelection) {
    // Apply settings to selected objects only
    config.setSelectedTextureEnabled(true);
    config.setSelectedTextureMode(RenderingConfig::TextureMode::Decal);
    // ... other selected object settings
} else {
    // Apply settings to all objects
    config.setTextureEnabled(true);
    config.setTextureMode(RenderingConfig::TextureMode::Decal);
    // ... other global settings
}
```

## Usage Examples

### Applying Texture Mode to Selected Objects
1. Select one or more objects in the scene
2. Click on a texture mode button (Decal, Modulate, Replace, Blend)
3. The texture mode will be applied only to the selected objects

### Applying Texture Mode to All Objects
1. Make sure no objects are selected (deselect all objects)
2. Click on a texture mode button
3. The texture mode will be applied to all objects in the scene

### Applying Transparency to Selected Objects
1. Select one or more objects
2. Use the transparency command or menu
3. Set transparency values in the dialog
4. Transparency will be applied only to selected objects

### Applying Transparency to All Objects
1. Make sure no objects are selected
2. Use the transparency command or menu
3. Set transparency values in the dialog
4. Transparency will be applied to all objects

## Benefits

1. **Flexible Rendering Control**: Users can apply different rendering settings to different objects
2. **Intuitive Workflow**: Selection-based rendering follows common CAD software patterns
3. **Consistent Behavior**: All rendering commands follow the same selection logic
4. **Backward Compatibility**: Existing functionality is preserved when no objects are selected
5. **Real-time Updates**: Changes are applied immediately with proper refresh mechanisms

## Testing and Feedback

The system includes built-in test feedback functionality that provides immediate visual and log feedback when rendering settings are applied:

### Visual Feedback
- **Texture Mode Commands**: Show message boxes indicating which objects were affected and the applied texture mode
- **Transparency Commands**: Show message boxes indicating which objects received transparency settings
- **Selection Status**: Clear indication of whether settings were applied to selected objects or all objects

### Log Feedback
- **Detailed Settings**: Complete rendering settings are logged when changes are applied
- **Object Count**: Number of objects updated is logged
- **Selection Status**: Whether objects were selected or all objects were affected
- **Material Properties**: Current material, texture, blend, and display settings

### Test Feedback Methods
The RenderingConfig class provides test feedback methods:

```cpp
// Get current selection status
std::string getCurrentSelectionStatus() const;

// Get detailed rendering settings
std::string getCurrentRenderingSettings() const;

// Show comprehensive test feedback
void showTestFeedback() const;
```

### Example Test Feedback Output
```
=== RenderingConfig Test Feedback ===
Selection status: Available (check OCCViewer for actual selection)
Current Rendering Settings:
- Material Diffuse: R=0.2 G=0.8 B=0.2
- Material Transparency: 0.0
- Texture Enabled: Yes
- Texture Mode: Decal
- Texture Color: R=1.0 G=0.0 B=0.0
- Blend Mode: None
- Display Mode: Solid
- Shading Mode: Smooth
=== End Test Feedback ===
```

### SceneManager Test Feedback
The SceneManager callback also provides test feedback:
```
=== Test Feedback: Updated 3 selected objects ===
```
or
```
=== Test Feedback: Updated 5 total objects ===
``` 