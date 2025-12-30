# Display Mode Switch Implementation Confirmation

## Overview

The display mode switching in `DisplayModeHandler` is now implemented using **three independent `SoSwitch` nodes**, matching the structure used in `DisplayModePreviewCanvas`. This provides fast mode switching by controlling visibility through switch `whichChild` properties rather than node manipulation.

## Architecture

### Three Independent Switches

The main view uses three separate `SoSwitch` nodes to control visibility:

1. **`m_surfaceSwitch`** - Controls surface geometry visibility
2. **`m_edgesSwitch`** - Controls edge geometry visibility  
3. **`m_pointsSwitch`** - Controls point view visibility

This matches the preview canvas structure documented in `DisplayModePreviewCanvasNodeStructure.md`.

## Implementation Flow

### 1. DisplayModeHandler::updateDisplayMode()

**Location**: `wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp`

**Key Code**:
```cpp
void DisplayModeHandler::updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                                           ModularEdgeComponent* edgeComponent,
                                           const Quantity_Color* originalDiffuseColor) {
    if (!coinNode) {
        return;
    }

    if (m_useSwitchMode && m_modeSwitch) {
        // New Switch structure: Three independent switches (surface, edges, points)
        // Update switch visibility for fast mode switching
        // Try both BREP and Mesh handlers (one will work depending on geometry type)
        if (m_brepHandler) {
            m_brepHandler->updateDisplayModeSwitches(coinNode, mode, edgeComponent);
        }
        if (m_meshHandler) {
            m_meshHandler->updateDisplayModeSwitches(coinNode, mode, edgeComponent);
        }
        return;
    }
    
    // ... Direct mode code (fallback for non-switch mode)
}
```

**Behavior**:
- When `m_useSwitchMode` is `true` and `m_modeSwitch` exists, delegates to `updateDisplayModeSwitches()`
- Falls back to direct node manipulation if switch mode is not available

### 2. BRepDisplayModeHandler::updateDisplayModeSwitches()

**Location**: `wxcoin/src/opencascade/geometry/helper/BRepDisplayModeHandler.cpp`

**Key Code**:
```cpp
void BRepDisplayModeHandler::updateDisplayModeSwitches(SoSeparator* coinNode,
                                                       RenderingConfig::DisplayMode mode,
                                                       ModularEdgeComponent* edgeComponent) {
    if (!coinNode || !m_useSwitchMode) {
        return;
    }
    
    // Find the three switches in coinNode (they should be direct children)
    SoSwitch* surfaceSwitch = nullptr;
    SoSwitch* edgesSwitch = nullptr;
    SoSwitch* pointsSwitch = nullptr;
    
    // ... Find switches in coinNode or use internal switches ...
    
    // Get configuration for this mode
    DisplayModeConfig config = DisplayModeConfigFactory::getConfig(mode, defaultContext);
    
    // Update switch visibility
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    
    // Update edges switch visibility
    bool showOriginalEdges = config.nodes.requireOriginalEdges && config.edges.originalEdge.enabled;
    int edgesSwitchValue = showOriginalEdges ? 0 : -1;
    edgesSwitch->whichChild.setValue(edgesSwitchValue);
    
    // Update points switch visibility
    int pointsSwitchValue = config.nodes.requirePoints ? 0 : -1;
    pointsSwitch->whichChild.setValue(pointsSwitchValue);
    
    // ... Handle edge extraction and appearance updates ...
}
```

**Behavior**:
- Locates the three switch nodes in `coinNode`
- Retrieves `DisplayModeConfig` for the target mode
- Sets `whichChild` property of each switch based on config requirements:
  - `0` = Show (child[0] is visible)
  - `-1` = Hide (no child visible)

### 3. MeshDisplayModeHandler::updateDisplayModeSwitches()

**Location**: `wxcoin/src/opencascade/geometry/helper/MeshDisplayModeHandler.cpp`

**Implementation**: Similar to `BRepDisplayModeHandler::updateDisplayModeSwitches()`, but handles mesh geometries instead of BREP shapes.

## Switch Visibility Logic

### Surface Switch (`m_surfaceSwitch`)
```cpp
int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
```
- **0**: Show surface geometry (`config.nodes.requireSurface == true`)
- **-1**: Hide surface geometry (`config.nodes.requireSurface == false`)

### Edges Switch (`m_edgesSwitch`)
```cpp
bool showOriginalEdges = config.nodes.requireOriginalEdges && config.edges.originalEdge.enabled;
int edgesSwitchValue = showOriginalEdges ? 0 : -1;
edgesSwitch->whichChild.setValue(edgesSwitchValue);
```
- **0**: Show edges (both `requireOriginalEdges` and `originalEdge.enabled` are true)
- **-1**: Hide edges

### Points Switch (`m_pointsSwitch`)
```cpp
int pointsSwitchValue = config.nodes.requirePoints ? 0 : -1;
pointsSwitch->whichChild.setValue(pointsSwitchValue);
```
- **0**: Show points (`config.nodes.requirePoints == true`)
- **-1**: Hide points

## Switch Initialization

The three switches are initialized in `handleDisplayMode()` methods:

### BRepDisplayModeHandler::initializeSwitchStructure()
- Creates `m_surfaceSwitch`, `m_edgesSwitch`, `m_pointsSwitch`
- Creates `m_surfaceNode`, `m_edgesNode`, `m_pointsNode`
- Moves existing geometry into appropriate nodes
- Adds switches to `coinNode`

### MeshDisplayModeHandler::initializeSwitchStructure()
- Similar structure for mesh geometries

## Node Structure

```
coinNode (SoSeparator)
├── m_surfaceSwitch (SoSwitch)
│   └── child[0]: m_surfaceNode (SoSeparator)
│       ├── SoLightModel
│       ├── SoDrawStyle
│       ├── SoMaterial
│       ├── SoShapeHints (optional)
│       ├── SoPolygonOffset (optional)
│       └── [Surface Geometry]
│
├── m_edgesSwitch (SoSwitch)
│   └── child[0]: m_edgesNode (SoSeparator)
│       ├── SoPolygonOffset (optional)
│       └── [Edge Nodes from ModularEdgeComponent]
│
└── m_pointsSwitch (SoSwitch)
    └── child[0]: m_pointsNode (SoSeparator)
        └── [Point View Nodes from PointViewBuilder]
```

## Configuration Source

The switch visibility is determined by `DisplayModeConfig`, retrieved via:
```cpp
DisplayModeConfig config = DisplayModeConfigFactory::getConfig(mode, defaultContext);
```

This ensures consistency with the preview canvas configuration system.

## Mode-Specific Behavior

### NoShading Mode
- `requireSurface = true` → `surfaceSwitch.whichChild = 0`
- `requireOriginalEdges = true` → `edgesSwitch.whichChild = 0` (if enabled)
- `requirePoints = false` → `pointsSwitch.whichChild = -1`

### Points Mode
- `requireSurface = false` → `surfaceSwitch.whichChild = -1`
- `requirePoints = true` → `pointsSwitch.whichChild = 0`

### Wireframe Mode
- `requireSurface = false` → `surfaceSwitch.whichChild = -1`
- `requireOriginalEdges = true` → `edgesSwitch.whichChild = 0` (if enabled)

### Solid Mode
- `requireSurface = true` → `surfaceSwitch.whichChild = 0`
- `requireOriginalEdges = true` → `edgesSwitch.whichChild = 0` (if enabled)

### FlatLines Mode
- `requireSurface = true` → `surfaceSwitch.whichChild = 0`
- `requireOriginalEdges = true` → `edgesSwitch.whichChild = 0` (if enabled)

### Transparent Mode
- `requireSurface = true` → `surfaceSwitch.whichChild = 0`
- `requireOriginalEdges = false` → `edgesSwitch.whichChild = -1`

### HiddenLine Mode
- `requireSurface = true` → `surfaceSwitch.whichChild = 0`
- `requireMeshEdges = true` → `edgesSwitch.whichChild = 0` (if enabled)

## Advantages of Switch Mode

1. **Fast Switching**: Changing `whichChild` is much faster than removing/adding nodes
2. **Consistent Structure**: Matches preview canvas architecture
3. **State Preservation**: State nodes (Material, DrawStyle, etc.) remain in scene graph
4. **Simplified Logic**: Visibility controlled by single property per switch

## Comparison with Direct Mode

### Switch Mode (Current Implementation)
- **Speed**: Fast (O(1) switch property update)
- **Structure**: Three independent switches
- **State Management**: State nodes rebuilt dynamically in `m_surfaceNode`
- **Use Case**: Main view display mode switching

### Direct Mode (Fallback)
- **Speed**: Slower (O(n) node removal/addition)
- **Structure**: Direct node manipulation
- **State Management**: Nodes removed and recreated
- **Use Case**: Legacy support, non-switch scenarios

## Verification

To verify switch mode is active:

1. Check `m_useSwitchMode` flag in `DisplayModeHandler`
2. Verify three `SoSwitch` nodes exist in `coinNode`
3. Confirm `updateDisplayModeSwitches()` is called (not direct mode code)
4. Monitor `whichChild` property changes during mode switching

## Conclusion

**Display mode switching is now implemented using switch code** (`updateDisplayModeSwitches()`), providing fast and efficient mode switching through three independent `SoSwitch` nodes that control surface, edges, and points visibility respectively. This architecture matches the preview canvas implementation and ensures consistent behavior across the application.







