# DisplayModeConfigDialog Node Data Structure Analysis

## Overview

`DisplayModeConfigDialog` provides a unified configuration interface for all display modes. It uses a data-driven architecture where each display mode is configured through a `DisplayModeConfig` structure that maps directly to Coin3D node requirements.

## Core Data Structure: DisplayModeConfig

```cpp
struct DisplayModeConfig {
    // ========== Node Requirements ==========
    struct NodeRequirements {
        bool requireSurface = false;         // Surface/faces geometry node (SoIndexedFaceSet)
        bool requireOriginalEdges = false;   // Original geometric edges (BREP topology)
        bool requireMeshEdges = false;       // Mesh edges (from triangulation)
        bool requirePoints = false;          // Vertex points (SoPointSet)
        bool surfaceWithPoints = false;      // Show surface together with points
    } nodes;
    
    // ========== Rendering Properties ==========
    struct RenderingProperties {
        enum class LightModel {
            BASE_COLOR,  // No lighting (SoLightModel::BASE_COLOR)
            PHONG        // Phong lighting (SoLightModel::PHONG)
        } lightModel;
        
        struct MaterialOverride {
            bool enabled = false;
            Quantity_Color ambientColor;
            Quantity_Color diffuseColor;
            Quantity_Color specularColor;
            Quantity_Color emissiveColor;
            double shininess = 0.0;
            double transparency = 0.0;
        } materialOverride;
        
        bool textureEnabled = false;
        RenderingConfig::BlendMode blendMode = RenderingConfig::BlendMode::None;
    } rendering;
    
    // ========== Edge Configuration ==========
    struct EdgeConfig {
        struct OriginalEdge {
            bool enabled = false;
            Quantity_Color color;
            double width = 1.0;
        } originalEdge;
        
        struct MeshEdge {
            bool enabled = false;
            Quantity_Color color;
            double width = 1.0;
            bool useEffectiveColor = false;  // Auto-adjust color for HiddenLine mode
        } meshEdge;
    } edges;
    
    // ========== Post-Processing ==========
    struct PostProcessing {
        struct PolygonOffset {
            bool enabled = false;
            float factor = 0.0f;  // Depth offset factor
            float units = 0.0f;   // Depth offset units
        } polygonOffset;
    } postProcessing;
};
```

## Configuration Dialog Structure

### Dialog Layout
```
DisplayModeConfigDialog
├── Left Panel (Notebook)
│   ├── No Shading Page
│   ├── Points Page
│   ├── Wireframe Page
│   ├── Solid Page
│   ├── Flat Lines Page
│   ├── Transparent Page
│   ├── Hidden Line Page
│   └── Custom Page
└── Right Panel (Preview Canvas)
```

### Each Mode Page Contains 4 Panels

#### 1. Node Requirements Panel
Controls which geometry nodes are required:
- **Require Surface**: Enable/disable surface geometry (SoIndexedFaceSet)
- **Require Original Edges**: Enable/disable BREP topology edges
- **Require Mesh Edges**: Enable/disable triangulation edges
- **Require Points**: Enable/disable vertex points (SoPointSet)

#### 2. Rendering Properties Panel
Controls rendering state nodes:
- **Light Model**: BASE_COLOR or PHONG (maps to SoLightModel)
- **Texture Enabled**: Enable texture mapping (SoTexture2)
- **Blend Mode**: None, Alpha, Additive, Multiply, Screen, Overlay
- **Material Override**:
  - Enabled checkbox
  - Ambient Color (SoMaterial::ambientColor)
  - Diffuse Color (SoMaterial::diffuseColor)
  - Specular Color (SoMaterial::specularColor)
  - Emissive Color (SoMaterial::emissiveColor)
  - Shininess slider (0-128.0, maps to SoMaterial::shininess)
  - Transparency slider (0-1.0, maps to SoMaterial::transparency)

#### 3. Edge Configuration Panel
Controls edge rendering:
- **Original Edge**:
  - Enabled checkbox
  - Color picker (SoLineSet color)
  - Width slider (0.1-10.0, maps to SoDrawStyle::lineWidth)
- **Mesh Edge**:
  - Enabled checkbox
  - Color picker
  - Width slider
  - Use Effective Color checkbox (for HiddenLine mode)

#### 4. Post-Processing Panel
Controls post-processing nodes:
- **Polygon Offset**:
  - Enabled checkbox
  - Factor slider (-10.0 to 10.0, maps to SoPolygonOffset::factor)
  - Units slider (-10.0 to 10.0, maps to SoPolygonOffset::units)

## Mode-Specific Configuration Visibility

Each mode shows/hides controls based on relevance:

### NoShading Mode
**Visible Controls:**
- Node Requirements: Surface ✓, Original Edges ✓
- Rendering: Light Model ✓, Material Override (Diffuse only) ✓
- Edges: Original Edge ✓
- Post-Processing: Polygon Offset ✓

**Hidden Controls:**
- Texture, Blend Mode
- Material: Ambient, Specular, Emissive, Shininess, Transparency
- Mesh Edges

**Config Mapping:**
```cpp
nodes.requireSurface = true
nodes.requireOriginalEdges = true
rendering.lightModel = BASE_COLOR
rendering.materialOverride.enabled = true
edges.originalEdge.enabled = true
```

### Points Mode
**Visible Controls:**
- Node Requirements: Surface ✓, Points ✓
- Rendering: Light Model ✓

**Hidden Controls:**
- All Material Override controls
- All Edge controls
- Post-Processing

**Config Mapping:**
```cpp
nodes.requireSurface = context.showSolidWithPointView
nodes.requirePoints = true
rendering.lightModel = BASE_COLOR
```

### Wireframe Mode
**Visible Controls:**
- Node Requirements: Surface ✓, Original Edges ✓
- Rendering: Light Model ✓, Material Override (enabled only) ✓
- Edges: Original Edge ✓
- Post-Processing: Polygon Offset ✓

**Hidden Controls:**
- Texture, Blend Mode
- Material color controls, Shininess, Transparency
- Mesh Edges

**Config Mapping:**
```cpp
nodes.requireSurface = false  // Only edges shown
nodes.requireOriginalEdges = true
rendering.lightModel = BASE_COLOR
edges.originalEdge.enabled = true
```

### Solid Mode
**Visible Controls:**
- **ALL** Node Requirements ✓
- **ALL** Rendering Properties ✓
- Edges: Original Edge (enabled checkbox disabled, controlled by requireOriginalEdges)
- Post-Processing: Polygon Offset ✓

**Special Behavior:**
- `originalEdge.enabled` is **synchronized** with `requireOriginalEdges`
- Original Edge checkbox is disabled (controlled by Node Requirements)

**Config Mapping:**
```cpp
nodes.requireSurface = true
nodes.requireOriginalEdges = true/false (user configurable)
rendering.lightModel = PHONG
rendering.textureEnabled = true/false
rendering.blendMode = user selectable
edges.originalEdge.enabled = nodes.requireOriginalEdges  // FORCED SYNC
```

### FlatLines Mode
**Visible Controls:**
- Node Requirements: Surface ✓, Original Edges ✓
- Rendering: Light Model ✓, Material Override (Shininess only) ✓
- Edges: Original Edge (enabled checkbox disabled, controlled by requireOriginalEdges)
- Post-Processing: Polygon Offset ✓

**Special Behavior:**
- `originalEdge.enabled` is **synchronized** with `requireOriginalEdges`
- Original Edge checkbox is disabled (controlled by Node Requirements)

**Config Mapping:**
```cpp
nodes.requireSurface = true
nodes.requireOriginalEdges = true
rendering.lightModel = PHONG
rendering.materialOverride.enabled = true
rendering.materialOverride.shininess = 30.0
edges.originalEdge.enabled = true  // FORCED SYNC
```

### Transparent Mode
**Visible Controls:**
- Node Requirements: Surface ✓
- Rendering: Light Model ✓, Blend Mode ✓, Material Override (Transparency only) ✓

**Hidden Controls:**
- All Edge controls
- Post-Processing

**Config Mapping:**
```cpp
nodes.requireSurface = true
rendering.lightModel = PHONG
rendering.blendMode = Alpha
rendering.materialOverride.transparency = 0.5 (default)
```

### HiddenLine Mode
**Visible Controls:**
- Node Requirements: Surface ✓, Mesh Edges ✓
- Rendering: Light Model ✓, Blend Mode ✓, Material Override (Ambient/Diffuse only) ✓
- Edges: Mesh Edge ✓
- Post-Processing: Polygon Offset ✓

**Hidden Controls:**
- Original Edges
- Material: Specular, Emissive, Shininess, Transparency

**Config Mapping:**
```cpp
nodes.requireSurface = true
nodes.requireMeshEdges = true
rendering.lightModel = BASE_COLOR
rendering.materialOverride.ambientColor = (1, 1, 1)  // White
rendering.materialOverride.diffuseColor = (1, 1, 1)  // White
edges.meshEdge.enabled = true
edges.meshEdge.useEffectiveColor = true  // Auto-adjust color
postProcessing.polygonOffset.enabled = true
postProcessing.polygonOffset.factor = 1.0
postProcessing.polygonOffset.units = 1.0
```

## Configuration to Node Mapping

### Node Requirements → Coin3D Nodes

| Config Field | Coin3D Node | Node Type |
|--------------|-------------|-----------|
| `requireSurface` | Surface geometry | SoIndexedFaceSet |
| `requireOriginalEdges` | Original edges | SoLineSet (via ModularEdgeComponent) |
| `requireMeshEdges` | Mesh edges | SoLineSet (via ModularEdgeComponent) |
| `requirePoints` | Point view | SoPointSet + SoCoordinate3 |

### Rendering Properties → Coin3D Nodes

| Config Field | Coin3D Node | Field Mapping |
|--------------|-------------|---------------|
| `lightModel = BASE_COLOR` | SoLightModel | `model = BASE_COLOR` |
| `lightModel = PHONG` | SoLightModel | `model = PHONG` |
| `materialOverride.enabled = true` | SoMaterial | Override material properties |
| `materialOverride.ambientColor` | SoMaterial | `ambientColor` |
| `materialOverride.diffuseColor` | SoMaterial | `diffuseColor` |
| `materialOverride.specularColor` | SoMaterial | `specularColor` |
| `materialOverride.emissiveColor` | SoMaterial | `emissiveColor` |
| `materialOverride.shininess` | SoMaterial | `shininess = value / 100.0` |
| `materialOverride.transparency` | SoMaterial | `transparency = value` |
| `textureEnabled = true` | SoTexture2 | Texture node added |
| `blendMode = Alpha` | SoShapeHints | Blending hints added |

### Edge Configuration → Coin3D Nodes

| Config Field | Coin3D Node | Field Mapping |
|--------------|-------------|---------------|
| `originalEdge.enabled = true` | SoLineSet | Original edges displayed |
| `originalEdge.color` | SoLineSet | Edge color |
| `originalEdge.width` | SoDrawStyle | `lineWidth` |
| `meshEdge.enabled = true` | SoLineSet | Mesh edges displayed |
| `meshEdge.color` | SoLineSet | Edge color |
| `meshEdge.width` | SoDrawStyle | `lineWidth` |

### Post-Processing → Coin3D Nodes

| Config Field | Coin3D Node | Field Mapping |
|--------------|-------------|---------------|
| `polygonOffset.enabled = true` | SoPolygonOffset | Depth offset node added |
| `polygonOffset.factor` | SoPolygonOffset | `factor` |
| `polygonOffset.units` | SoPolygonOffset | `units` |

## Configuration Synchronization Rules

### Critical Synchronization (Solid & FlatLines Modes)

In **Solid** and **FlatLines** modes, there's a forced synchronization:

```cpp
// In updateConfigFromControls() and updateControls()
if (mode == RenderingConfig::DisplayMode::FlatLines || 
    mode == RenderingConfig::DisplayMode::Solid) {
    // FORCE SYNC: originalEdge.enabled must match requireOriginalEdges
    controls.config.edges.originalEdge.enabled = controls.config.nodes.requireOriginalEdges;
    
    // Disable checkbox (user controls via requireOriginalEdges)
    controls.originalEdgeEnabled->Enable(false);
}
```

**Rationale**: In these modes, original edges are an integral part of the display mode, not an optional feature.

### Control Visibility Rules

Controls are shown/hidden based on mode relevance:

1. **Node Requirements**: Always visible (user controls what geometry to show)
2. **Rendering Properties**: 
   - Light Model: Always visible
   - Texture: Only Solid mode
   - Blend Mode: Solid, Transparent, HiddenLine modes
   - Material Override: Mode-specific fields shown
3. **Edge Configuration**:
   - Original Edge: NoShading, Wireframe, Solid, FlatLines modes
   - Mesh Edge: HiddenLine mode only
4. **Post-Processing**:
   - Polygon Offset: All modes except Points and Transparent

## Configuration Flow

### Loading Configuration
```
1. Dialog opens
2. For each mode:
   - Load default config from DisplayModeConfigFactory::getConfig()
   - Store in m_modeControls[mode].config
   - Update UI controls from config
   - Apply visibility rules (updateModeVisibility)
```

### Updating Configuration
```
1. User changes control value
2. updateConfigFromControls() called
3. Config structure updated from control values
4. Preview updated (updatePreview())
```

### Applying Configuration
```
1. User clicks Apply/OK
2. saveConfigForMode() called for each mode
3. Config stored in m_modeControls
4. Config can be retrieved via getConfig(mode)
```

## Default Configurations (from DisplayModeConfigFactory)

### NoShading
```cpp
nodes.requireSurface = true
nodes.requireOriginalEdges = true
rendering.lightModel = BASE_COLOR
rendering.materialOverride.enabled = true
rendering.materialOverride.ambientColor = (0, 0, 0)
rendering.materialOverride.specularColor = (0, 0, 0)
edges.originalEdge.enabled = true
```

### Points
```cpp
nodes.requireSurface = context.showSolidWithPointView
nodes.requirePoints = true
rendering.lightModel = BASE_COLOR
```

### Wireframe
```cpp
nodes.requireSurface = false
nodes.requireOriginalEdges = true
rendering.lightModel = BASE_COLOR
edges.originalEdge.enabled = true
```

### Solid
```cpp
nodes.requireSurface = true
nodes.requireOriginalEdges = false  // User configurable
rendering.lightModel = PHONG
edges.originalEdge.enabled = false  // User configurable
```

### FlatLines
```cpp
nodes.requireSurface = true
nodes.requireOriginalEdges = true
rendering.lightModel = PHONG
rendering.materialOverride.enabled = true
rendering.materialOverride.shininess = 30.0
edges.originalEdge.enabled = true
```

### Transparent
```cpp
nodes.requireSurface = true
rendering.lightModel = PHONG
rendering.blendMode = Alpha
rendering.materialOverride.transparency = 0.5
```

### HiddenLine
```cpp
nodes.requireSurface = true
nodes.requireMeshEdges = true
rendering.lightModel = BASE_COLOR
rendering.materialOverride.ambientColor = (1, 1, 1)
rendering.materialOverride.diffuseColor = (1, 1, 1)
edges.meshEdge.enabled = true
postProcessing.polygonOffset.enabled = true
postProcessing.polygonOffset.factor = 1.0
postProcessing.polygonOffset.units = 1.0
```

## Key Implementation Details

### 1. Mode Controls Storage
```cpp
std::map<RenderingConfig::DisplayMode, ModeControls> m_modeControls;

struct ModeControls {
    wxPanel* page;
    DisplayModeConfig config;
    
    // Node Requirements controls
    wxCheckBox* requireSurface;
    wxCheckBox* requireOriginalEdges;
    wxCheckBox* requireMeshEdges;
    wxCheckBox* requirePoints;
    
    // Rendering Properties controls
    wxChoice* lightModel;
    wxCheckBox* textureEnabled;
    wxChoice* blendMode;
    wxCheckBox* materialOverrideEnabled;
    wxButton* materialAmbientColor;
    wxButton* materialDiffuseColor;
    wxButton* materialSpecularColor;
    wxButton* materialEmissiveColor;
    wxSlider* materialShininess;
    wxStaticText* materialShininessLabel;
    wxSlider* materialTransparency;
    wxStaticText* materialTransparencyLabel;
    
    // Edge Configuration controls
    wxCheckBox* originalEdgeEnabled;
    wxButton* originalEdgeColor;
    wxSlider* originalEdgeWidth;
    wxStaticText* originalEdgeWidthLabel;
    wxCheckBox* meshEdgeEnabled;
    wxButton* meshEdgeColor;
    wxSlider* meshEdgeWidth;
    wxStaticText* meshEdgeWidthLabel;
    wxCheckBox* meshEdgeUseEffectiveColor;
    
    // Post-Processing controls
    wxCheckBox* polygonOffsetEnabled;
    wxSlider* polygonOffsetFactor;
    wxStaticText* polygonOffsetFactorLabel;
    wxSlider* polygonOffsetUnits;
    wxStaticText* polygonOffsetUnitsLabel;
};
```

### 2. Control Value Mapping

**Slider to Config Value:**
- Shininess: `sliderValue / 10.0` (range: 0-128.0)
- Transparency: `sliderValue / 100.0` (range: 0-1.0)
- Edge Width: `sliderValue / 10.0` (range: 0.1-10.0)
- Polygon Offset: `sliderValue / 10.0` (range: -10.0 to 10.0)

**Color Conversion:**
- wxColour → Quantity_Color: `(r/255.0, g/255.0, b/255.0)`
- Quantity_Color → wxColour: `(r*255, g*255, b*255)`

### 3. Preview Integration

The preview canvas (`DisplayModePreviewCanvas`) receives the current config and renders it:
```cpp
void DisplayModeConfigDialog::updatePreview() {
    RenderingConfig::DisplayMode mode = getModeFromPageIndex(currentPage);
    updateConfigFromControls(mode);
    DisplayModeConfig config = m_modeControls[mode].config;
    m_previewCanvas->updateDisplayMode(mode, config);
}
```

## Relationship to DisplayModeHandler

The `DisplayModeConfig` structure maps directly to the node structure built by `DisplayModeHandler`:

1. **Node Requirements** → Determines which geometry nodes are built
2. **Rendering Properties** → Maps to state nodes (SoLightModel, SoMaterial, etc.)
3. **Edge Configuration** → Controls ModularEdgeComponent edge display
4. **Post-Processing** → Adds SoPolygonOffset for depth handling

The configuration dialog provides a user-friendly interface to modify these settings, which are then applied when building the scene graph.







