# DisplayMode Node Structure Analysis

## Overview

DisplayModeHandler supports two rendering modes:
1. **Switch Mode**: Uses `SoSwitch` node to switch between pre-built mode nodes (faster switching)
2. **Direct Mode**: Directly modifies nodes on `coinNode` (more flexible)

## Node Structure for Each Display Mode

### Common Base Structure (coinNode: SoSeparator)

All display modes share the same base structure:
```
coinNode (SoSeparator)
├── [State Nodes] (added/removed dynamically)
│   ├── SoLightModel
│   ├── SoDrawStyle
│   ├── SoMaterial
│   ├── SoShapeHints (for Transparent mode)
│   └── SoPolygonOffset
├── [Geometry Nodes] (preserved during mode switch)
│   └── Surface geometry (SoIndexedFaceSet, etc.)
├── [Edge Nodes] (from ModularEdgeComponent)
│   ├── Original edges (SoLineSet)
│   └── Mesh edges (SoLineSet)
└── [Point View Nodes] (SoSeparator containing SoPointSet)
```

---

## 1. NoShading Mode (switchIndex = 0)

### State Configuration (from DisplayModeStateManager)
- `showSurface = true`
- `showOriginalEdges = true`
- `lightingEnabled = false`
- `wireframeMode = false`
- `surfaceDisplayMode = NoShading`

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = BASE_COLOR (no lighting)
├── SoDrawStyle
│   └── style = FILLED
├── SoMaterial
│   ├── ambientColor = (0, 0, 0)
│   ├── diffuseColor = (preserved from context)
│   ├── specularColor = (0, 0, 0)
│   ├── emissiveColor = (0, 0, 0)
│   └── shininess = 0.0
├── SoPolygonOffset (if showSurface)
│   └── default values
├── [Surface Geometry] (SoIndexedFaceSet)
└── [Original Edges] (from ModularEdgeComponent)
    └── SoLineSet (black edges, width 1.0)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 0)
    └── child[0]: SoSeparator (NoShading state node)
        ├── SoLightModel (BASE_COLOR)
        ├── SoDrawStyle (FILLED)
        ├── SoMaterial (no lighting)
        └── [Surface Geometry]
```

---

## 2. Points Mode (switchIndex = 1)

### State Configuration
- `showPoints = true`
- `showSurface = context.showSolidWithPointView` (optional)
- `lightingEnabled = false`
- `showOriginalEdges = false`
- `showMeshEdges = false`

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = BASE_COLOR
├── SoDrawStyle
│   └── style = POINTS
├── SoMaterial
│   └── (point color settings)
├── [Surface Geometry] (if showSolidWithPointView)
└── [Point View Node] (SoSeparator)
    ├── SoCoordinate3 (vertex coordinates)
    └── SoPointSet (point rendering)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 1)
    └── child[1]: SoSeparator (Points state node)
        ├── SoLightModel (BASE_COLOR)
        ├── SoDrawStyle (POINTS)
        ├── SoMaterial
        └── [Point View Node]
```

---

## 3. Wireframe Mode (switchIndex = 2)

### State Configuration
- `showSurface = false`
- `showOriginalEdges = true`
- `wireframeMode = true`
- `lightingEnabled = false`

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = BASE_COLOR
├── SoDrawStyle
│   └── style = LINES
├── SoMaterial
│   ├── ambientColor = (0, 0, 0)
│   ├── diffuseColor = (edge color)
│   ├── specularColor = (0, 0, 0)
│   └── shininess = 0.0
├── [Original Edges] (from ModularEdgeComponent)
│   └── SoLineSet (wireframe edges)
└── [No Surface Geometry] (hidden)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 2)
    └── child[2]: SoSeparator (Wireframe state node)
        ├── SoLightModel (BASE_COLOR)
        ├── SoDrawStyle (LINES)
        ├── SoMaterial (edge color)
        └── [Original Edges]
```

---

## 4. FlatLines Mode (switchIndex = 4)

### State Configuration
- `showSurface = true`
- `showOriginalEdges = true`
- `lightingEnabled = true`
- `wireframeMode = false`
- `shininess = 30.0`

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = PHONG
├── SoDrawStyle
│   └── style = FILLED
├── SoMaterial
│   ├── ambientColor = (from context)
│   ├── diffuseColor = (from context)
│   ├── specularColor = (from context)
│   └── shininess = 30.0
├── SoPolygonOffset (if showSurface)
│   └── default values
├── [Surface Geometry] (SoIndexedFaceSet)
└── [Original Edges] (from ModularEdgeComponent)
    └── SoLineSet (black edges, width 1.0)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 4)
    └── child[4]: SoSeparator (FlatLines state node)
        ├── SoLightModel (PHONG)
        ├── SoDrawStyle (FILLED)
        ├── SoMaterial (with lighting)
        ├── SoPolygonOffset
        ├── [Surface Geometry]
        └── [Original Edges]
```

---

## 5. Solid Mode (switchIndex = 3, default)

### State Configuration
- `showSurface = true`
- `showOriginalEdges = false` (can be enabled separately)
- `lightingEnabled = true`
- `wireframeMode = false`

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = PHONG
├── SoDrawStyle
│   └── style = FILLED
├── SoMaterial
│   ├── ambientColor = (from context)
│   ├── diffuseColor = (from context)
│   ├── specularColor = (from context)
│   └── shininess = (from context)
├── SoPolygonOffset (if smoothNormals enabled)
│   ├── factor = 1.0
│   └── units = 1.0
├── [Surface Geometry] (SoIndexedFaceSet)
└── [Optional: Original Edges] (if enabled)
    └── SoLineSet (with PolygonOffset for Z-fighting)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 3)
    └── child[3]: SoSeparator (Solid state node)
        ├── SoLightModel (PHONG)
        ├── SoDrawStyle (FILLED)
        ├── SoMaterial (with lighting)
        ├── SoPolygonOffset (optional)
        └── [Surface Geometry]
```

---

## 6. Transparent Mode (switchIndex = 5)

### State Configuration
- `showSurface = true`
- `lightingEnabled = true`
- `transparency = 0.5` (default)
- `blendMode = Alpha`

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = PHONG
├── SoDrawStyle
│   └── style = FILLED
├── SoMaterial
│   ├── ambientColor = (from context)
│   ├── diffuseColor = (from context)
│   ├── specularColor = (from context)
│   └── transparency = 0.5
├── SoShapeHints (for blending)
│   ├── faceType = UNKNOWN_FACE_TYPE
│   └── vertexOrdering = UNKNOWN_ORDERING
├── SoPolygonOffset (if showSurface)
│   └── default values
└── [Surface Geometry] (SoIndexedFaceSet)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 5)
    └── child[5]: SoSeparator (Transparent state node)
        ├── SoLightModel (PHONG)
        ├── SoDrawStyle (FILLED)
        ├── SoMaterial (with transparency)
        ├── SoShapeHints (blending)
        ├── SoPolygonOffset
        └── [Surface Geometry]
```

---

## 7. HiddenLine Mode (switchIndex = 6)

### State Configuration
- `showSurface = true`
- `showMeshEdges = true`
- `lightingEnabled = false`
- `surfaceDisplayMode = HiddenLine`
- `surfaceDiffuseColor = (1, 1, 1)` (white)

### Node Structure (Direct Mode)
```
coinNode (SoSeparator)
├── SoLightModel
│   └── model = BASE_COLOR
├── SoDrawStyle
│   └── style = FILLED
├── SoMaterial
│   ├── ambientColor = (1, 1, 1)
│   ├── diffuseColor = (1, 1, 1)
│   ├── specularColor = (0, 0, 0)
│   └── shininess = 0.0
├── SoPolygonOffset (special offset for hidden line)
│   ├── factor = 1.0 (push back)
│   └── units = 1.0
├── [Surface Geometry] (SoIndexedFaceSet, white)
└── [Mesh Edges] (from ModularEdgeComponent)
    └── SoLineSet (black edges, visible edges only)
```

### Switch Mode Structure
```
coinNode (SoSeparator)
└── SoSwitch (whichChild = 6)
    └── child[6]: SoSeparator (HiddenLine state node)
        ├── SoLightModel (BASE_COLOR)
        ├── SoDrawStyle (FILLED)
        ├── SoMaterial (white, no lighting)
        ├── SoPolygonOffset (push back)
        ├── [Surface Geometry] (white)
        └── [Mesh Edges] (visible edges)
```

---

## Node Ordering (Critical)

The order of nodes in `coinNode` is critical for correct rendering:

### Standard Order (from updateDisplayMode)
1. **SoLightModel** - Lighting model (BASE_COLOR or PHONG)
2. **SoDrawStyle** - Drawing style (FILLED, LINES, POINTS)
3. **SoMaterial** - Material properties (colors, shininess, transparency)
4. **SoShapeHints** - (Optional, for Transparent mode blending)
5. **SoPolygonOffset** - (Optional, for Z-fighting prevention)
6. **[Surface Geometry]** - Geometry nodes (preserved)
7. **[Edge Nodes]** - Edge rendering (from ModularEdgeComponent)
8. **[Point View Nodes]** - Point rendering (if enabled)

### Edge Node Ordering (from ModularEdgeComponent)
- Original edges are added after surface geometry
- Mesh edges are added after original edges
- PolygonOffset may be added before edges for Z-fighting prevention

---

## Switch Mode vs Direct Mode

### Switch Mode (m_useSwitchMode = true)
- **Advantage**: Fast mode switching (just change whichChild index)
- **Disadvantage**: All modes must be pre-built
- **Structure**: 
  ```
  coinNode
  └── SoSwitch
      ├── child[0]: NoShading node
      ├── child[1]: Points node
      ├── child[2]: Wireframe node
      ├── child[3]: Solid node
      ├── child[4]: FlatLines node
      ├── child[5]: Transparent node
      └── child[6]: HiddenLine node
  ```

### Direct Mode (m_useSwitchMode = false)
- **Advantage**: More flexible, can modify individual nodes
- **Disadvantage**: Slower switching (must remove/add nodes)
- **Structure**: Nodes directly added to coinNode

---

## Node Removal Strategy

When switching modes in Direct Mode, the following nodes are removed:
1. **State Nodes**: SoDrawStyle, SoMaterial, SoLightModel, SoPolygonOffset, SoShapeHints, SoTexture2
2. **Point View Nodes**: SoSeparator containing SoPointSet or SoCoordinate3
3. **HiddenLine Nodes**: SoSeparator containing PolygonModeNode
4. **Edge Nodes**: Removed via ModularEdgeComponent::cleanupEdgeNodes()

**Preserved Nodes**:
- **Geometry Nodes**: Surface geometry (SoIndexedFaceSet, etc.)
- **Switch Node**: If exists (for Switch mode)

---

## Key Implementation Details

1. **Material Color Preservation**: 
   - `originalDiffuseColor` parameter preserves diffuse color across mode switches
   - Other material properties (ambient, specular, emissive) are reset per mode

2. **Edge Display**:
   - Controlled by `ModularEdgeComponent`
   - Edge nodes are added/removed dynamically based on `updateState.showOriginalEdges` and `updateState.showMeshEdges`

3. **Z-Fighting Prevention**:
   - `SoPolygonOffset` used for Solid mode with smooth normals
   - Special offset for HiddenLine mode (push surface back)
   - Edge offset for Solid mode with original edges enabled

4. **Blending**:
   - `SoShapeHints` required for Transparent mode
   - `blendMode = Alpha` for transparency rendering







