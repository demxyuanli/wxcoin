# DisplayModePreviewCanvas Node Data Structure Analysis

## Overview

`DisplayModePreviewCanvas` is a preview canvas used in the `DisplayModeConfigDialog` to show real-time preview of display mode configurations. It uses Coin3D for rendering and maintains a simplified scene graph structure optimized for preview purposes.

## Complete Scene Graph Structure

```
m_sceneRoot (SoSeparator) - Root of scene graph
├── m_camera (SoPerspectiveCamera) - View camera
├── SoDirectionalLight - Scene lighting
├── m_lightModel (SoLightModel) - Lighting model (BASE_COLOR or PHONG)
│
├── m_surfaceSwitch (SoSwitch) - Controls surface visibility
│   └── child[0]: m_geometryRoot (SoSeparator)
│       └── m_surfaceNode (SoSeparator)
│           ├── m_drawStyle (SoDrawStyle) - Drawing style (FILLED/LINES/POINTS)
│           ├── m_material (SoMaterial) - Material properties
│           ├── m_shapeHints (SoShapeHints) - Shape hints for transparency
│           ├── m_polygonOffset (SoPolygonOffset) - Depth offset for Z-fighting
│           └── [Surface Geometry] (SoIndexedFaceSet from STEP file)
│
├── m_edgesSwitch (SoSwitch) - Controls edge visibility
│   └── child[0]: m_edgesNode (SoSeparator)
│       ├── [SoPolygonOffset] (optional, for edge Z-fighting)
│       └── [Edge Nodes from ModularEdgeComponent]
│           ├── Original edges (SoLineSet)
│           └── Mesh edges (SoLineSet)
│
└── m_pointsSwitch (SoSwitch) - Controls point visibility
    └── child[0]: m_pointsNode (SoSeparator)
        └── [Point View Nodes from PointViewBuilder]
            ├── SoCoordinate3 (vertex coordinates)
            └── SoPointSet (point rendering)
```

## Node Initialization Order

### 1. Scene Root Setup (`initializeScene()`)
```cpp
m_sceneRoot = new SoSeparator;
m_sceneRoot->ref();
```

### 2. Camera Setup (`setupCamera()`)
```cpp
m_camera = new SoPerspectiveCamera;
m_camera->ref();
// Position: rotated 45° around Y-axis, then rotated around X-axis
// Focal distance: 10.0
m_sceneRoot->addChild(m_camera);
```

### 3. Lighting Setup (`setupLighting()`)
```cpp
SoDirectionalLight* light = new SoDirectionalLight;
// Direction: follows camera orientation
light->direction = camera->orientation * (0, 0, -1)
m_sceneRoot->addChild(light);

m_lightModel = new SoLightModel;
m_lightModel->ref();
m_sceneRoot->addChild(m_lightModel);
```

### 4. Material Setup (`setupMaterial()`)
```cpp
m_material = new SoMaterial;
m_material->ref();

m_drawStyle = new SoDrawStyle;
m_drawStyle->ref();

m_shapeHints = new SoShapeHints;
m_shapeHints->ref();
```

### 5. Geometry Creation (`createGeometry()`)

#### 5.1 Geometry Root and Surface Node
```cpp
m_geometryRoot = new SoSeparator;
m_geometryRoot->ref();

m_surfaceNode = new SoSeparator;
m_surfaceNode->ref();
m_geometryRoot->addChild(m_surfaceNode);

// CRITICAL ORDER: DrawStyle -> Material -> ShapeHints -> PolygonOffset -> Geometry
m_surfaceNode->addChild(m_drawStyle);
m_surfaceNode->addChild(m_material);
m_surfaceNode->addChild(m_shapeHints);

m_polygonOffset = new SoPolygonOffset;
m_polygonOffset->ref();
m_surfaceNode->addChild(m_polygonOffset);
```

#### 5.2 Edge and Point Nodes
```cpp
m_edgesNode = new SoSeparator;
m_edgesNode->ref();

m_pointsNode = new SoSeparator;
m_pointsNode->ref();
```

#### 5.3 Switch Nodes
```cpp
m_surfaceSwitch = new SoSwitch;
m_surfaceSwitch->ref();
m_surfaceSwitch->addChild(m_geometryRoot);
m_sceneRoot->addChild(m_surfaceSwitch);

m_edgesSwitch = new SoSwitch;
m_edgesSwitch->ref();
m_edgesSwitch->addChild(m_edgesNode);
m_sceneRoot->addChild(m_edgesSwitch);

m_pointsSwitch = new SoSwitch;
m_pointsSwitch->ref();
m_pointsSwitch->addChild(m_pointsNode);
m_sceneRoot->addChild(m_pointsSwitch);
```

#### 5.4 Geometry Loading
- Loads STEP file (`modpreview.stp`) from `config/samples/`
- Converts to Coin3D geometry using `OCCBrepConverter::convertToCoin3D()`
- **CRITICAL**: Removes Material and ShapeHints nodes from loaded geometry to prevent override conflicts
- Adds geometry to `m_surfaceNode`

## Configuration Update Flow (`updateGeometryFromConfig()`)

### 1. Light Model Update
```cpp
if (config.rendering.lightModel == BASE_COLOR) {
    m_lightModel->model.setValue(SoLightModel::BASE_COLOR);
} else {
    m_lightModel->model.setValue(SoLightModel::PHONG);
}
```

### 2. Material Override Update
```cpp
if (config.rendering.materialOverride.enabled) {
    m_material->diffuseColor = config.materialOverride.diffuseColor;
    m_material->ambientColor = config.materialOverride.ambientColor;
    m_material->specularColor = config.materialOverride.specularColor;
    m_material->emissiveColor = config.materialOverride.emissiveColor;
    m_material->shininess = config.materialOverride.shininess;
    m_material->transparency = config.materialOverride.transparency;
}
```

### 3. Polygon Offset Update
```cpp
m_polygonOffset->factor = config.postProcessing.polygonOffset.factor;
m_polygonOffset->units = config.postProcessing.polygonOffset.units;
m_polygonOffset->on = config.postProcessing.polygonOffset.enabled;
```

### 4. Shape Hints Update (for Transparency)
```cpp
if (transparency > 0.0) {
    m_shapeHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
    m_shapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
} else {
    m_shapeHints->faceType = SoShapeHints::SOLID;
    m_shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
}
```

### 5. Surface Switch Control
```cpp
int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
// 0 = show surface, -1 = hide surface
```

### 6. Edge Display Update

#### 6.1 Edge Node Clearing
```cpp
m_edgesNode->removeAllChildren();
```

#### 6.2 Edge Extraction Logic
```cpp
bool showOriginalEdges = config.nodes.requireOriginalEdges && 
                         config.edges.originalEdge.enabled;
bool showMeshEdges = config.nodes.requireMeshEdges && 
                     config.edges.meshEdge.enabled;

if (showOriginalEdges) {
    // Extract original edges from BREP shape
    m_edgeComponent->extractOriginalEdges(
        m_shape,
        samplingDensity,  // Adaptive based on mesh deflection
        minLength,
        false,
        config.edges.originalEdge.color,
        config.edges.originalEdge.width,
        ...
    );
    // Apply appearance (color/width) even if edges already exist
    m_edgeComponent->applyAppearanceToEdgeNode(
        EdgeType::Original,
        config.edges.originalEdge.color,
        config.edges.originalEdge.width,
        0
    );
    m_edgeComponent->setEdgeDisplayType(EdgeType::Original, true);
}

if (showMeshEdges) {
    // Extract mesh edges from triangulation
    Quantity_Color edgeColor = config.edges.meshEdge.color;
    if (config.edges.meshEdge.useEffectiveColor) {
        // Auto-adjust: use black if color is too light
        if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
            edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        }
    }
    m_edgeComponent->extractMeshEdges(*m_mesh, edgeColor, config.edges.meshEdge.width);
    m_edgeComponent->setEdgeDisplayType(EdgeType::Mesh, true);
}
```

#### 6.3 Edge Polygon Offset
```cpp
if (config.postProcessing.polygonOffset.enabled) {
    SoPolygonOffset* edgePolygonOffset = new SoPolygonOffset();
    edgePolygonOffset->factor = config.postProcessing.polygonOffset.factor;
    edgePolygonOffset->units = config.postProcessing.polygonOffset.units;
    edgePolygonOffset->styles = SoPolygonOffset::LINES;
    m_edgesNode->addChild(edgePolygonOffset);
} else {
    // Default: negative offset to bring edges forward
    SoPolygonOffset* edgePolygonOffset = new SoPolygonOffset();
    edgePolygonOffset->factor = -1.0f;
    edgePolygonOffset->units = -1.0f;
    edgePolygonOffset->styles = SoPolygonOffset::LINES;
    m_edgesNode->addChild(edgePolygonOffset);
}

m_edgeComponent->updateEdgeDisplay(m_edgesNode);
```

#### 6.4 Edge Switch Control
```cpp
if (showOriginalEdges || showMeshEdges) {
    m_edgesSwitch->whichChild.setValue(0);  // Show edges
} else {
    m_edgesSwitch->whichChild.setValue(-1);  // Hide edges
}
```

### 7. Point View Update
```cpp
m_pointsNode->removeAllChildren();

if (config.nodes.requirePoints && m_pointViewBuilder && m_mesh) {
    m_pointViewBuilder->createPointViewRepresentation(
        m_pointsNode,
        *m_mesh,
        defaultContext.display
    );
    m_pointsSwitch->whichChild.setValue(0);  // Show points
} else {
    m_pointsSwitch->whichChild.setValue(-1);  // Hide points
}
```

## Key Node Properties

### SoLightModel (m_lightModel)
- **Location**: Scene root (affects entire scene)
- **Values**:
  - `BASE_COLOR`: No lighting, direct color display
  - `PHONG`: Standard Phong lighting model
- **Update**: Changed based on `config.rendering.lightModel`

### SoDrawStyle (m_drawStyle)
- **Location**: `m_surfaceNode` (affects surface geometry only)
- **Values**:
  - `FILLED`: Solid surface rendering
  - `LINES`: Wireframe rendering
  - `POINTS`: Point rendering
- **Note**: Not directly updated from config (handled by display mode)

### SoMaterial (m_material)
- **Location**: `m_surfaceNode` (affects surface geometry only)
- **Properties**:
  - `ambientColor`: Ambient light color
  - `diffuseColor`: Diffuse light color (main color)
  - `specularColor`: Specular highlight color
  - `emissiveColor`: Emissive color
  - `shininess`: Material shininess (0-128)
  - `transparency`: Transparency (0-1)
- **Update**: Set from `config.rendering.materialOverride` when enabled

### SoShapeHints (m_shapeHints)
- **Location**: `m_surfaceNode`
- **Purpose**: Controls face ordering and transparency rendering
- **Values**:
  - **Opaque**: `SOLID` + `COUNTERCLOCKWISE` (better performance)
  - **Transparent**: `UNKNOWN_FACE_TYPE` + `UNKNOWN_ORDERING` (proper blending)
- **Update**: Changed based on transparency value

### SoPolygonOffset (m_polygonOffset)
- **Location**: `m_surfaceNode`
- **Purpose**: Prevents Z-fighting between surface and edges
- **Properties**:
  - `factor`: Depth offset factor
  - `units`: Depth offset units
  - `on`: Enable/disable offset
- **Update**: Set from `config.postProcessing.polygonOffset`

### SoSwitch Nodes

#### m_surfaceSwitch
- **Controls**: Surface geometry visibility
- **Values**:
  - `0`: Show surface (`config.nodes.requireSurface == true`)
  - `-1`: Hide surface (`config.nodes.requireSurface == false`)

#### m_edgesSwitch
- **Controls**: Edge geometry visibility
- **Values**:
  - `0`: Show edges (when `showOriginalEdges || showMeshEdges`)
  - `-1`: Hide edges

#### m_pointsSwitch
- **Controls**: Point view visibility
- **Values**:
  - `0`: Show points (`config.nodes.requirePoints == true`)
  - `-1`: Hide points

## Node Ordering (Critical)

### Surface Node Order
The order of nodes in `m_surfaceNode` is **critical** for correct rendering:

```
m_surfaceNode (SoSeparator)
├── SoDrawStyle          (1st: Drawing style)
├── SoMaterial           (2nd: Material properties)
├── SoShapeHints         (3rd: Shape hints for transparency)
├── SoPolygonOffset      (4th: Depth offset)
└── [Surface Geometry]   (5th: Actual geometry)
```

**Rationale**:
1. **DrawStyle** must come first to set rendering mode
2. **Material** must come before geometry to apply material properties
3. **ShapeHints** must come before geometry for transparency to work correctly
4. **PolygonOffset** must come before geometry to affect depth sorting
5. **Geometry** comes last to be rendered with all state applied

### Edge Node Order
```
m_edgesNode (SoSeparator)
├── SoPolygonOffset (optional, for edge Z-fighting)
└── [Edge Nodes from ModularEdgeComponent]
    ├── Original edges (SoLineSet)
    └── Mesh edges (SoLineSet)
```

**Note**: Edge polygon offset uses `SoPolygonOffset::LINES` style to only affect lines.

## Configuration to Node Mapping

| Config Field | Node | Property | Notes |
|--------------|------|----------|-------|
| `nodes.requireSurface` | `m_surfaceSwitch` | `whichChild` | 0 = show, -1 = hide |
| `nodes.requireOriginalEdges` | `m_edgeComponent` | `setEdgeDisplayType(Original)` | Combined with `edges.originalEdge.enabled` |
| `nodes.requireMeshEdges` | `m_edgeComponent` | `setEdgeDisplayType(Mesh)` | Combined with `edges.meshEdge.enabled` |
| `nodes.requirePoints` | `m_pointsSwitch` | `whichChild` | 0 = show, -1 = hide |
| `rendering.lightModel` | `m_lightModel` | `model` | BASE_COLOR or PHONG |
| `rendering.materialOverride.enabled` | `m_material` | All properties | When enabled, override material |
| `rendering.materialOverride.diffuseColor` | `m_material` | `diffuseColor` | Main color |
| `rendering.materialOverride.transparency` | `m_material` | `transparency` | Also affects `m_shapeHints` |
| `edges.originalEdge.enabled` | `m_edgeComponent` | `setEdgeDisplayType(Original)` | Combined with `nodes.requireOriginalEdges` |
| `edges.originalEdge.color` | `m_edgeComponent` | `applyAppearanceToEdgeNode()` | Edge color |
| `edges.originalEdge.width` | `m_edgeComponent` | `applyAppearanceToEdgeNode()` | Edge width |
| `edges.meshEdge.enabled` | `m_edgeComponent` | `setEdgeDisplayType(Mesh)` | Combined with `nodes.requireMeshEdges` |
| `edges.meshEdge.color` | `m_edgeComponent` | `extractMeshEdges()` | Edge color (with effective color logic) |
| `edges.meshEdge.useEffectiveColor` | `m_edgeComponent` | `extractMeshEdges()` | Auto-adjust color for HiddenLine |
| `postProcessing.polygonOffset.enabled` | `m_polygonOffset` | `on` | Enable/disable offset |
| `postProcessing.polygonOffset.factor` | `m_polygonOffset` | `factor` | Depth offset factor |
| `postProcessing.polygonOffset.units` | `m_polygonOffset` | `units` | Depth offset units |

## Edge Display Logic

### Original Edges Display Condition
```cpp
bool showOriginalEdges = config.nodes.requireOriginalEdges && 
                         config.edges.originalEdge.enabled;
```

**Both conditions must be true**:
1. Node requirement: `requireOriginalEdges = true`
2. Edge configuration: `originalEdge.enabled = true`

### Mesh Edges Display Condition
```cpp
bool showMeshEdges = config.nodes.requireMeshEdges && 
                     config.edges.meshEdge.enabled;
```

**Both conditions must be true**:
1. Node requirement: `requireMeshEdges = true`
2. Edge configuration: `meshEdge.enabled = true`

### Edge Sampling Density (Adaptive)
```cpp
double samplingDensity = 80.0;  // Default
double minLength = 0.01;        // Default

if (m_meshParams.deflection > 0.0) {
    minLength = m_meshParams.deflection * 0.5;
    samplingDensity = 1.0 / m_meshParams.deflection;
    // Clamp to reasonable range
    if (samplingDensity < 20.0) samplingDensity = 20.0;
    if (samplingDensity > 200.0) samplingDensity = 200.0;
}
```

**Rationale**: Edge sampling density adapts to surface mesh quality to ensure edges match surface detail level.

## Transparency Rendering

### Material Transparency
```cpp
double transparency = config.rendering.materialOverride.enabled 
    ? config.rendering.materialOverride.transparency 
    : 0.0;
```

### Shape Hints Configuration
```cpp
if (transparency > 0.0) {
    m_shapeHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
    m_shapeHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
} else {
    m_shapeHints->faceType = SoShapeHints::SOLID;
    m_shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
}
```

### Render Action Configuration
```cpp
if (hasTransparency) {
    renderAction.setNumPasses(optimalPasses);  // 2-3 passes
    renderAction.setTransparencyType(
        isComplexScene 
            ? SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND
            : SoGLRenderAction::SORTED_OBJECT_BLEND
    );
}
```

## Geometry Loading Process

### 1. STEP File Search
Searches for `modpreview.stp` in:
1. `{CWD}/config/samples/modpreview.stp`
2. `{EXE_DIR}/config/samples/modpreview.stp`
3. `{PROJECT_ROOT}/config/samples/modpreview.stp`

### 2. Geometry Conversion
```cpp
TopoDS_Shape shape = OCCBrepConverter::loadFromSTEP(stepPathStr);
TriangleMesh mesh = processor->convertToMesh(shape, m_meshParams);
SoSeparator* stepGeometry = OCCBrepConverter::convertToCoin3D(shape, deflection);
```

### 3. Node Cleanup
**CRITICAL**: Removes Material and ShapeHints nodes from loaded geometry:
```cpp
// Remove Material nodes (we have our own)
if (child->isOfType(SoMaterial::getClassTypeId())) {
    sep->removeChild(i);
}

// Remove ShapeHints nodes (we have our own)
if (child->isOfType(SoShapeHints::getClassTypeId())) {
    sep->removeChild(i);
}
```

**Rationale**: Prevents loaded geometry's material/shape hints from overriding our configuration.

### 4. Geometry Addition
```cpp
m_surfaceNode->addChild(stepGeometry);
```

## Rendering Process (`onPaint()`)

### 1. OpenGL Setup
```cpp
glViewport(0, 0, width, height);
glClearColor(0.85f, 0.9f, 0.95f, 1.0f);  // Light blue-gray background
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
glEnable(GL_DEPTH_TEST);
glEnable(GL_LIGHTING);
```

### 2. Transparency Configuration
```cpp
if (hasTransparency) {
    renderAction.setNumPasses(optimalPasses);
    renderAction.setTransparencyType(
        isComplexScene 
            ? SORTED_OBJECT_SORTED_TRIANGLE_BLEND
            : SORTED_OBJECT_BLEND
    );
}
```

### 3. Blend Mode Handling
```cpp
if (hasBlendMode && !hasTransparency) {
    glEnable(GL_BLEND);
    switch (config.rendering.blendMode) {
        case Alpha: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
        case Additive: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break;
        // ... other blend modes
    }
}
```

### 4. Scene Rendering
```cpp
renderAction.apply(m_sceneRoot);
```

## Key Differences from Main DisplayModeHandler

### 1. Simplified Structure
- **Preview**: Uses Switch nodes for visibility control
- **Main**: Uses direct node manipulation or Switch mode

### 2. Separate Edge/Point Nodes
- **Preview**: Edges and points in separate Switch nodes
- **Main**: Edges and points integrated into main scene graph

### 3. Material Override Location
- **Preview**: Material node in `m_surfaceNode` (affects only surface)
- **Main**: Material node in scene root or mode-specific location

### 4. Geometry Loading
- **Preview**: Loads single STEP file (`modpreview.stp`)
- **Main**: Uses existing geometry from OCCViewer

### 5. Configuration Source
- **Preview**: Receives `DisplayModeConfig` directly from dialog
- **Main**: Builds config from `DisplayModeRenderState` and context

## Node Reference Management

All nodes are reference-counted using Coin3D's `ref()`/`unref()` system:

```cpp
// Creation
m_sceneRoot = new SoSeparator;
m_sceneRoot->ref();  // Increment reference count

// Cleanup (in destructor)
if (m_sceneRoot) {
    m_sceneRoot->unref();  // Decrement reference count
}
```

**Important**: Nodes added as children are automatically referenced by parent, but explicit `ref()` is needed for member variables to prevent premature deletion.

## Summary

`DisplayModePreviewCanvas` maintains a **simplified, preview-optimized scene graph**:

1. **Three Switch nodes** control visibility of surface, edges, and points
2. **State nodes** (Material, DrawStyle, ShapeHints, PolygonOffset) are in `m_surfaceNode`
3. **Configuration updates** modify node properties directly (no node removal/recreation)
4. **Edge display** uses `ModularEdgeComponent` for consistent edge rendering
5. **Transparency** is handled through Material transparency + ShapeHints + render action passes

This structure is optimized for **fast configuration updates** and **real-time preview** in the configuration dialog.







