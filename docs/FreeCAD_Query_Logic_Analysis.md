# FreeCAD Query Logic Analysis

## Overview
FreeCAD uses a sophisticated query/picking system based on Coin3D (OpenInventor) for interactive element selection. The system supports picking faces, edges, and vertices with precise highlighting.

## Core Components

### 1. Coin3D Ray Picking
**Location**: `View3DInventorViewer::getPointOnRay()`, `SoRayPickAction`

- Uses `SoRayPickAction` to cast a ray from screen coordinates into the 3D scene
- Returns `SoPickedPoint` containing:
  - `SoPath*` - Path through the scene graph to the picked object
  - `SoDetail*` - Detailed information about what was picked (face, edge, vertex)
  - 3D coordinates of the intersection point

### 2. ViewProvider::getElementPicked()
**Location**: `FreeCAD/src/Gui/ViewProvider.cpp:915`

```cpp
bool ViewProvider::getElementPicked(const SoPickedPoint *pp, std::string &subname) const {
    if(!isSelectable())
        return false;
    // Try extensions first
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for(Gui::ViewProviderExtension* ext : vector) {
        if(ext->extensionGetElementPicked(pp,subname))
            return true;
    }
    // Fallback to base implementation
    subname = getElement(pp?pp->getDetail():nullptr);
    return true;
}
```

**Key Points**:
- Extracts sub-element name (e.g., "Face5", "Edge12", "Vertex3") from `SoPickedPoint`
- Uses extension system for specialized handling
- Calls `getElement()` to convert `SoDetail` to string name

### 3. ViewProvider::getDetailPath()
**Location**: `FreeCAD/src/Gui/ViewProvider.cpp:927`

```cpp
bool ViewProvider::getDetailPath(const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const {
    if(pcRoot->findChild(pcModeSwitch) < 0)
        return false;
    if(append) {
        pPath->append(pcRoot);
        pPath->append(pcModeSwitch);
    }
    // Try extensions first
    auto vector = getExtensionsDerivedFromType<Gui::ViewProviderExtension>();
    for(Gui::ViewProviderExtension* ext : vector) {
        if(ext->extensionGetDetailPath(subname,pPath,det))
            return true;
    }
    // Fallback to base implementation
    det = getDetail(subname);
    return true;
}
```

**Key Points**:
- Converts sub-element name (e.g., "Face5") back to Coin3D path and detail
- Used for highlighting: need path + detail to apply highlight action
- Builds path from root through mode switch to target element

### 4. SoFCUnifiedSelection - Selection System
**Location**: `FreeCAD/src/Gui/Selection/SoFCUnifiedSelection.cpp`

**Main Flow**:

1. **getPickedList()** (line 240):
   - Gets all picked points from `SoHandleEventAction`
   - For each point, calls `vp->getElementPicked(pp, element)` to get sub-element name
   - Returns list of `PickedInfo` with ViewProvider, element name, and picked point

2. **Preselection (Hover)** (line 347):
   ```cpp
   // Get detail path for highlighting
   vp->getDetailPath(subName, detailPath, true, detail);
   // Apply highlight action
   SoHighlightElementAction highlightAction;
   highlightAction.setHighlighted(true);
   highlightAction.setColor(this->colorHighlight.getValue());
   highlightAction.setElement(detail);
   highlightAction.apply(pathToHighlight);
   ```

3. **Selection (Click)** (line 421):
   ```cpp
   // Get detail path for selection highlighting
   vp->getDetailPath(subName, detailPath, true, detail);
   // Apply selection action
   SoSelectionElementAction selectionAction(type);
   selectionAction.setColor(this->colorSelection.getValue());
   selectionAction.setElement(detail);
   selectionAction.apply(detailPath);
   ```

### 5. Selection Singleton
**Location**: `FreeCAD/src/Gui/Selection/Selection.cpp`

- Global selection state management
- Notifies observers of selection changes
- Manages preselection and selection lists
- Uses signals/slots for decoupled communication

## Key Design Patterns

### 1. Two-Way Conversion
- **Picking → Name**: `getElementPicked()` converts `SoPickedPoint` → "Face5"
- **Name → Highlighting**: `getDetailPath()` converts "Face5" → `SoPath` + `SoDetail`

### 2. Extension System
- ViewProvider uses extension pattern for specialized behavior
- Extensions can override `extensionGetElementPicked()` and `extensionGetDetailPath()`
- Allows different geometry types (Part, PartDesign, Assembly) to have custom logic

### 3. Action-Based Highlighting
- Uses Coin3D actions (`SoHighlightElementAction`, `SoSelectionElementAction`)
- Actions traverse scene graph and modify nodes based on detail
- Efficient: no need to rebuild scene graph, just apply action

### 4. Path-Based Scene Graph Navigation
- `SoFullPath` represents path from root to target node
- Path includes: Root → ModeSwitch → ... → TargetElement
- Detail identifies specific sub-element within the target node

## Comparison with Current Implementation

### Current Implementation (wxcoin)
- ✅ Uses `SoRayPickAction` for picking (same as FreeCAD)
- ✅ Extracts `SoFaceDetail` from picked point
- ✅ Uses face index mapping to convert triangle index → geometry face ID
- ✅ Generates sub-element names in FreeCAD style ("Face5")
- ❌ Missing `getDetailPath()` - cannot convert name back to path+detail
- ❌ Uses separate highlight nodes instead of action-based highlighting
- ❌ No unified selection system like `SoFCUnifiedSelection`

### Recommended Improvements

1. **Implement `getDetailPath()` equivalent**:
   - Convert "Face5" → Coin3D path + detail
   - Needed for action-based highlighting

2. **Use Action-Based Highlighting**:
   - Replace `FaceHighlightManager` with `SoHighlightElementAction` / `SoSelectionElementAction`
   - More efficient and matches FreeCAD approach

3. **Implement Unified Selection Node**:
   - Create `SoFCUnifiedSelection`-like node
   - Handles preselection and selection automatically
   - Integrates with scene graph

4. **Support Edge and Vertex Picking**:
   - Currently only supports face picking
   - Need `SoLineDetail` and `SoPointDetail` handling

## Key Files Reference

### FreeCAD Source
- `FreeCAD/src/Gui/ViewProvider.cpp` - Base ViewProvider implementation
- `FreeCAD/src/Gui/Selection/SoFCUnifiedSelection.cpp` - Unified selection system
- `FreeCAD/src/Gui/Selection/Selection.cpp` - Selection singleton
- `FreeCAD/src/Gui/View3DInventorViewer.cpp` - Ray picking implementation

### Current Implementation
- `src/opencascade/viewer/PickingService.cpp` - Ray picking service
- `src/mod/ViewProvider.cpp` - ViewProvider implementation
- `src/mod/FaceHighlightManager.cpp` - Face highlighting (should use actions)
- `src/mod/Selection.cpp` - Selection state management





