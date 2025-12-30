# Switch and CoinNode Data Structure Analysis

## Overview

This document analyzes the data structures of `SoSwitch` and `coinNode` (SoSeparator) in two key components:
1. **DisplayModePreviewCanvas** - Preview canvas for display mode configuration
2. **DisplayModeHandler** - Display mode management handler

---

## 1. DisplayModePreviewCanvas Structure

### Scene Graph Hierarchy

```
m_sceneRoot (SoSeparator) - Root node
├── m_camera (SoPerspectiveCamera)
├── SoDirectionalLight
├── m_lightModel (SoLightModel) - Global lighting model
│
├── m_surfaceSwitch (SoSwitch) - Controls surface visibility
│   └── child[0]: m_geometryRoot (SoSeparator)
│       └── m_surfaceNode (SoSeparator)
│           ├── m_drawStyle (SoDrawStyle)
│           ├── m_material (SoMaterial)
│           ├── m_shapeHints (SoShapeHints)
│           ├── m_polygonOffset (SoPolygonOffset)
│           └── [Surface Geometry] (SoIndexedFaceSet from STEP)
│
├── m_edgesSwitch (SoSwitch) - Controls edge visibility
│   └── child[0]: m_edgesNode (SoSeparator)
│       ├── [SoPolygonOffset] (optional, for Z-fighting)
│       └── [Edge Nodes from ModularEdgeComponent]
│
└── m_pointsSwitch (SoSwitch) - Controls point visibility
    └── child[0]: m_pointsNode (SoSeparator)
        └── [Point View Nodes from PointViewBuilder]
```

### Key Characteristics

#### Three Independent Switches Architecture

```235:248:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
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

**Switch Control Values:**
- `whichChild = 0`: Show the child node
- `whichChild = -1` (SO_SWITCH_NONE): Hide all children

#### Surface Node Structure

```202:228:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
void DisplayModePreviewCanvas::createGeometry() {
    m_geometryRoot = new SoSeparator;
    m_geometryRoot->ref();
    
    m_surfaceNode = new SoSeparator;
    m_surfaceNode->ref();
    m_geometryRoot->addChild(m_surfaceNode);
    
    // Add rendering state nodes to surface node in correct order
    // Order is critical for proper transparency rendering:
    // LightModel -> DrawStyle -> Material -> ShapeHints -> PolygonOffset -> Geometry
    // Note: LightModel is already in scene root, so we add the rest here
    if (m_drawStyle) {
        m_surfaceNode->addChild(m_drawStyle);
    }
    if (m_material) {
        m_surfaceNode->addChild(m_material);
    }
    if (m_shapeHints) {
        m_surfaceNode->addChild(m_shapeHints);
    }
    
    // Create polygon offset node and add it to surface node
    m_polygonOffset = new SoPolygonOffset;
    m_polygonOffset->ref();
    m_surfaceNode->addChild(m_polygonOffset);
```

**Critical Node Ordering:**
1. `SoDrawStyle` - Drawing style (FILLED/LINES/POINTS)
2. `SoMaterial` - Material properties (colors, transparency)
3. `SoShapeHints` - Shape hints for transparency rendering
4. `SoPolygonOffset` - Depth offset for Z-fighting prevention
5. `[Surface Geometry]` - Actual geometry nodes

#### Switch Control Logic

```509:512:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("updateGeometryFromConfig: Surface switch set to " + std::to_string(surfaceSwitchValue) + 
              " (requireSurface=" + (config.nodes.requireSurface ? "true" : "false") + ")");
```

```610:628:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
        m_edgesSwitch->whichChild.setValue(0);
    } else {
        m_edgesSwitch->whichChild.setValue(-1);
    }
    
    m_pointsNode->removeAllChildren();
    
    if (config.nodes.requirePoints && m_pointViewBuilder && m_mesh) {
        GeometryRenderContext defaultContext;
        defaultContext.display.pointViewColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);
        defaultContext.display.pointViewSize = 3.0;
        defaultContext.display.pointViewShape = 0;
        
        m_pointViewBuilder->createPointViewRepresentation(m_pointsNode, *m_mesh, defaultContext.display);
        m_pointsSwitch->whichChild.setValue(0);
        LOG_INF_S("Points view created: " + std::to_string(m_mesh->vertices.size()) + " points");
    } else {
        m_pointsSwitch->whichChild.setValue(-1);
    }
```

---

## 2. DisplayModeHandler Structure

### Two Operating Modes

#### Mode 1: Switch Mode (New Architecture)

**Structure:**
```
coinNode (SoSeparator)
├── m_surfaceSwitch (SoSwitch)
│   └── child[0]: m_surfaceNode (SoSeparator)
│       ├── [State Nodes]
│       └── [Surface Geometry]
├── m_edgesSwitch (SoSwitch)
│   └── child[0]: m_edgesNode (SoSeparator)
│       └── [Edge Nodes]
└── m_pointsSwitch (SoSwitch)
    └── child[0]: m_pointsNode (SoSeparator)
        └── [Point View Nodes]
```

**Detection Logic:**

```125:149:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Check if three independent switches exist in coinNode (new architecture)
    // This is more reliable than checking m_modeSwitch
    SoSwitch* surfaceSwitch = nullptr;
    SoSwitch* edgesSwitch = nullptr;
    SoSwitch* pointsSwitch = nullptr;
    int switchCount = 0;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (child && child->isOfType(SoSwitch::getClassTypeId())) {
            SoSwitch* sw = static_cast<SoSwitch*>(child);
            if (switchCount == 0) {
                surfaceSwitch = sw;
            } else if (switchCount == 1) {
                edgesSwitch = sw;
            } else if (switchCount == 2) {
                pointsSwitch = sw;
            }
            ++switchCount;
        }
    }
    
    // Use Switch mode if:
    // 1. Config says useSwitchMode=true, AND
    // 2. Three switches exist in coinNode (or will be created by handlers)
    bool switchesExist = (surfaceSwitch && edgesSwitch && pointsSwitch);
    bool shouldUseSwitchMode = configUseSwitchMode && (switchesExist || m_useSwitchMode);
```

**Switch Mode Update:**

```152:169:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    if (shouldUseSwitchMode) {
        // New Switch structure: Three independent switches (surface, edges, points)
        // Update switch visibility for fast mode switching
        LOG_INF_S("DisplayModeHandler::updateDisplayMode: Switching to mode=" + displayModeToString(mode) + 
                  " using Switch mode (config.useSwitchMode=" + std::string(configUseSwitchMode ? "true" : "false") +
                  ", switchesExist=" + std::string(switchesExist ? "true" : "false") + 
                  ", found " + std::to_string(switchCount) + " switches in coinNode)");
        
        // Try both BREP and Mesh handlers (one will work depending on geometry type)
        if (m_brepHandler) {
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Calling BREP handler for switch update");
            m_brepHandler->updateDisplayModeSwitches(coinNode, mode, edgeComponent);
        }
        if (m_meshHandler) {
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Calling Mesh handler for switch update");
            m_meshHandler->updateDisplayModeSwitches(coinNode, mode, edgeComponent);
        }
        return;
    }
```

#### Mode 2: Direct Mode (Legacy Architecture)

**Structure:**
```
coinNode (SoSeparator)
├── SoLightModel
├── SoDrawStyle
├── SoMaterial
├── SoShapeHints (optional, for transparency)
├── SoPolygonOffset (optional)
├── [Surface Geometry] (preserved)
├── [Edge Nodes] (from ModularEdgeComponent)
└── [Point View Nodes] (SoSeparator with SoPointSet)
```

**Node Removal Strategy:**

```186:261:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Step 2: Reset only state nodes (preserve geometry nodes)
    // Collect state nodes to remove safely (avoid index shifting issues)
    std::vector<SoNode*> stateNodesToRemove;
    std::vector<SoNode*> pointViewNodesToRemove;
    std::vector<SoNode*> hiddenLineNodesToRemove;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        // Keep Switch node if it exists (for Switch mode)
        if (child->isOfType(SoSwitch::getClassTypeId())) {
            continue;
        }
        
        // Remove only state nodes, preserve geometry nodes
        if (child->isOfType(SoDrawStyle::getClassTypeId()) ||
            child->isOfType(SoMaterial::getClassTypeId()) ||
            child->isOfType(SoLightModel::getClassTypeId()) ||
            child->isOfType(SoPolygonOffset::getClassTypeId()) ||
            child->isOfType(SoShapeHints::getClassTypeId()) ||
            child->isOfType(SoTexture2::getClassTypeId())) {
            stateNodesToRemove.push_back(child);
        }
        
        // CRITICAL FIX: Preserve geometry nodes (mesh geometry for pure mesh models)
        // Check if this node contains geometry before considering it for removal
        if (nodeManager.containsGeometryNode(child)) {
            continue;  // Preserve geometry nodes
        }
        
        // Detect and remove point view nodes (SoSeparator containing SoPointSet or SoCoordinate3)
        // Also detect HiddenLine pass nodes (SoSeparator with PolygonModeNode)
        if (child->isOfType(SoSeparator::getClassTypeId())) {
            SoSeparator* sep = static_cast<SoSeparator*>(child);
            if (!sep) continue;  // Safety check
            
            bool isPointViewNode = false;
            bool isHiddenLineNode = false;
            
            // CRITICAL: Initialize PolygonModeNode class if not already initialized (before loop)
            if (PolygonModeNode::getClassTypeId() == SoType::badType()) {
                PolygonModeNode::initClass();
            }
            SoType polygonModeType = PolygonModeNode::getClassTypeId();
            bool polygonModeTypeValid = (polygonModeType != SoType::badType());
            
            int numChildren = sep->getNumChildren();
            for (int j = 0; j < numChildren; ++j) {
                SoNode* subChild = sep->getChild(j);
                if (!subChild) continue;
                
                try {
                    if (subChild->isOfType(SoPointSet::getClassTypeId())) {
                        isPointViewNode = true;
                        break;
                    }
                    if (subChild->isOfType(SoCoordinate3::getClassTypeId())) {
                        isPointViewNode = true;
                        break;
                    }
                    // Check for PolygonModeNode (HiddenLine mode)
                    if (polygonModeTypeValid && subChild->isOfType(polygonModeType)) {
                        isHiddenLineNode = true;
                        break;
                    }
                } catch (...) {
                    // Safety: Skip invalid nodes
                    continue;
                }
            }
            if (isPointViewNode) {
                pointViewNodesToRemove.push_back(child);
            }
            if (isHiddenLineNode) {
                hiddenLineNodesToRemove.push_back(child);
            }
        }
    }
```

**Node Addition Order (Critical):**

```401:495:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Step 5: Add nodes in correct order (matching applyRenderState)
    // Order: LightModel -> DrawStyle -> Material -> BlendHints -> PolygonOffset
    
    // Step 5.1: Add LightModel node for proper lighting control
    // Use BASE_COLOR for no-shading modes (NoShading, HiddenLine), PHONG for others
    SoLightModel* lightModel = new SoLightModel();
    lightModel->ref();
    if (!updateState.lightingEnabled || updateState.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading) {
        lightModel->model.setValue(SoLightModel::BASE_COLOR);  // No lighting, direct color
    } else {
        lightModel->model.setValue(SoLightModel::PHONG);  // Standard Phong lighting
    }
    coinNode->addChild(lightModel);
    lightModel->unref();

    // Step 5.2: Create DrawStyle node (was deleted by resetAllRenderStates)
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->ref();
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::Points:
        drawStyle->style.setValue(SoDrawStyle::POINTS);
        break;
    case RenderingConfig::DisplayMode::Wireframe:
        drawStyle->style.setValue(SoDrawStyle::LINES);
        break;
    case RenderingConfig::DisplayMode::FlatLines:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::Solid:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::Transparent:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::HiddenLine:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    default:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    }
    coinNode->addChild(drawStyle);
    drawStyle->unref();

    // Step 5.3: Create Material node based on render state (was deleted by resetAllRenderStates)
    SoMaterial* material = new SoMaterial();
    material->ref();
    
    // Apply material colors from updateState (which was set by setRenderStateForMode)
    Standard_Real r, g, b;
    
    updateState.surfaceAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
    material->ambientColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    updateState.surfaceDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
    material->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    updateState.surfaceSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
    material->specularColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    updateState.surfaceEmissiveColor.Values(r, g, b, Quantity_TOC_RGB);
    material->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    material->shininess.setValue(static_cast<float>(updateState.shininess / 100.0));
    material->transparency.setValue(static_cast<float>(updateState.transparency));
    
    coinNode->addChild(material);
    material->unref();

    // Step 5.4: Add BlendHints for Transparent mode
    if (updateState.blendMode == RenderingConfig::BlendMode::Alpha && updateState.transparency > 0.0) {
        SoShapeHints* blendHints = new SoShapeHints();
        blendHints->ref();
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        coinNode->addChild(blendHints);
        blendHints->unref();
    }

    // Step 5.5: Add PolygonOffset for modes that render surface
    // HiddenLine mode needs special offset (push back), other surface modes use default
    if (updateState.showSurface) {
        SoPolygonOffset* polygonOffset = new SoPolygonOffset();
        polygonOffset->ref();
        if (mode == RenderingConfig::DisplayMode::HiddenLine) {
            polygonOffset->factor.setValue(1.0f);  // Push surface back
            polygonOffset->units.setValue(1.0f);
        }
        // For other modes (Solid, Transparent, etc.), use default PolygonOffset values
        coinNode->addChild(polygonOffset);
        polygonOffset->unref();
    }
```

---

## 3. Key Differences

### DisplayModePreviewCanvas
- **Fixed Structure**: Always uses three independent switches
- **Simplified**: Optimized for preview purposes
- **Direct Control**: Switches are directly managed by the canvas
- **Single Geometry Source**: Loads from STEP file

### DisplayModeHandler
- **Dual Mode Support**: Supports both Switch mode and Direct mode
- **Dynamic Detection**: Automatically detects which mode to use
- **Flexible**: Can work with existing or new scene graphs
- **Multiple Geometry Sources**: Supports BREP and Mesh geometries

---

## 4. Switch Control Values

### SoSwitch::whichChild Values

| Value | Meaning | Usage |
|-------|---------|-------|
| `0` | Show first child (index 0) | Show the geometry |
| `-1` (SO_SWITCH_NONE) | Hide all children | Hide the geometry |
| `n` (n > 0) | Show child at index n | Multiple mode switching (not used in current implementation) |

### Switch Mode Decision Logic

```67:74:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Read switch mode configuration from RenderingConfig
    // Default to true (Switch mode) if config not available
    RenderingConfig& config = RenderingConfig::getInstance();
    m_useSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    LOG_INF_S("DisplayModeHandler::DisplayModeHandler: Initialized with useSwitchMode=" + 
              std::string(m_useSwitchMode ? "true" : "false") + " (from config)");
```

---

## 5. Node Lifecycle Management

### Reference Counting

Both programs use Coin3D's reference counting system:

```126:127:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    m_sceneRoot = new SoSeparator;
    m_sceneRoot->ref();
```

```235:237:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    m_surfaceSwitch = new SoSwitch;
    m_surfaceSwitch->ref();
    m_surfaceSwitch->addChild(m_geometryRoot);
```

**Cleanup:**

```77:118:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    if (m_sceneRoot) {
        m_sceneRoot->unref();
    }
    if (m_surfaceNode) {
        m_surfaceNode->unref();
    }
    if (m_edgesNode) {
        m_edgesNode->unref();
    }
    if (m_pointsNode) {
        m_pointsNode->unref();
    }
    if (m_geometryRoot) {
        m_geometryRoot->unref();
    }
    if (m_camera) {
        m_camera->unref();
    }
    if (m_material) {
        m_material->unref();
    }
    if (m_drawStyle) {
        m_drawStyle->unref();
    }
    if (m_lightModel) {
        m_lightModel->unref();
    }
    if (m_shapeHints) {
        m_shapeHints->unref();
    }
    if (m_polygonOffset) {
        m_polygonOffset->unref();
    }
    if (m_surfaceSwitch) {
        m_surfaceSwitch->unref();
    }
    if (m_edgesSwitch) {
        m_edgesSwitch->unref();
    }
    if (m_pointsSwitch) {
        m_pointsSwitch->unref();
    }
```

---

## 6. Critical Implementation Notes

### Node Ordering is Critical

The order of nodes in Coin3D scene graphs affects rendering:
1. **State nodes** (LightModel, DrawStyle, Material) must come before geometry
2. **ShapeHints** must come before geometry for transparency to work
3. **PolygonOffset** must come before geometry for Z-fighting prevention

### Switch vs Direct Mode Trade-offs

| Aspect | Switch Mode | Direct Mode |
|--------|-------------|-------------|
| **Performance** | Fast (just change whichChild) | Slower (remove/add nodes) |
| **Memory** | Higher (all modes pre-built) | Lower (only current mode) |
| **Flexibility** | Lower (pre-defined modes) | Higher (dynamic changes) |
| **Complexity** | Higher (switch management) | Lower (direct manipulation) |

### Geometry Preservation

In Direct Mode, geometry nodes are **always preserved** during mode switching:

```208:212:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
        // CRITICAL FIX: Preserve geometry nodes (mesh geometry for pure mesh models)
        // Check if this node contains geometry before considering it for removal
        if (nodeManager.containsGeometryNode(child)) {
            continue;  // Preserve geometry nodes
        }
```

Only state nodes (DrawStyle, Material, etc.) are removed and recreated.

---

## 7. Summary

### DisplayModePreviewCanvas
- Uses **three independent SoSwitch nodes** (surface, edges, points)
- Fixed structure optimized for preview
- Direct switch control via `whichChild` property

### DisplayModeHandler
- Supports **two architectures**:
  - **Switch Mode**: Three independent switches (new, faster)
  - **Direct Mode**: Direct node manipulation (legacy, more flexible)
- Automatically detects and uses appropriate mode
- Preserves geometry nodes during mode switching

Both implementations follow Coin3D best practices for scene graph organization and node lifecycle management.


