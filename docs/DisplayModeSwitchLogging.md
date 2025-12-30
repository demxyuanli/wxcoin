# Display Mode Switch Logging

## Overview

Detailed logging has been added to track display mode switching behavior, including switch visibility updates, configuration loading, and edge/point display control. This helps debug display mode switching issues and verify correct behavior.

## Logging Locations

### 1. DisplayModeHandler::updateDisplayMode()

**File**: `wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp`

**Log Messages**:

#### Switch Mode Entry
```
DisplayModeHandler::updateDisplayMode: Switching to mode={ModeName} using Switch mode
DisplayModeHandler::updateDisplayMode: Calling BREP handler for switch update
DisplayModeHandler::updateDisplayMode: Calling Mesh handler for switch update
```

#### Direct Mode Entry
```
DisplayModeHandler::updateDisplayMode: Switching to mode={ModeName} using Direct mode (m_useSwitchMode={true|false}, m_modeSwitch={valid|null})
```

**When Logged**: 
- At the start of `updateDisplayMode()` when switching display modes
- Indicates whether Switch mode or Direct mode is being used

---

### 2. BRepDisplayModeHandler::updateDisplayModeSwitches()

**File**: `wxcoin/src/opencascade/geometry/helper/BRepDisplayModeHandler.cpp`

**Log Messages**:

#### Initialization Check
```
BRepDisplayModeHandler::updateDisplayModeSwitches: Starting switch update for mode={modeIndex}
BRepDisplayModeHandler::updateDisplayModeSwitches: Found {count} switch nodes in coinNode
BRepDisplayModeHandler::updateDisplayModeSwitches: Using internal m_surfaceSwitch
BRepDisplayModeHandler::updateDisplayModeSwitches: Using internal m_edgesSwitch
BRepDisplayModeHandler::updateDisplayModeSwitches: Using internal m_pointsSwitch
```

#### Error Cases
```
BRepDisplayModeHandler::updateDisplayModeSwitches: Invalid parameters (coinNode={valid|null}, m_useSwitchMode={true|false})
BRepDisplayModeHandler::updateDisplayModeSwitches: Switches not initialized (surface={valid|null}, edges={valid|null}, points={valid|null})
```

#### Configuration Loading
```
BRepDisplayModeHandler::updateDisplayModeSwitches: Config loaded - requireSurface={true|false}, requireOriginalEdges={true|false}, requirePoints={true|false}, originalEdge.enabled={true|false}
```

#### Switch Visibility Updates
```
BRepDisplayModeHandler::updateDisplayModeSwitches: Surface switch: {oldValue} -> {newValue} (requireSurface={true|false})
BRepDisplayModeHandler::updateDisplayModeSwitches: Edges switch: {oldValue} -> {newValue} (requireOriginalEdges={true|false}, originalEdge.enabled={true|false}, showOriginalEdges={true|false})
BRepDisplayModeHandler::updateDisplayModeSwitches: Points switch: {oldValue} -> {newValue} (requirePoints={true|false})
BRepDisplayModeHandler::updateDisplayModeSwitches: Switch update completed successfully
```

**When Logged**:
- When updating switch visibility for BREP geometries
- Shows switch value changes (0 = visible, -1 = hidden)
- Includes configuration values that determine visibility

---

### 3. MeshDisplayModeHandler::updateDisplayModeSwitches()

**File**: `wxcoin/src/opencascade/geometry/helper/MeshDisplayModeHandler.cpp`

**Log Messages**:

#### Initialization Check
```
MeshDisplayModeHandler::updateDisplayModeSwitches: Starting switch update for mode={modeIndex}
MeshDisplayModeHandler::updateDisplayModeSwitches: Found {count} switch nodes in coinNode
MeshDisplayModeHandler::updateDisplayModeSwitches: Using internal m_surfaceSwitch
MeshDisplayModeHandler::updateDisplayModeSwitches: Using internal m_edgesSwitch
MeshDisplayModeHandler::updateDisplayModeSwitches: Using internal m_pointsSwitch
```

#### Error Cases
```
MeshDisplayModeHandler::updateDisplayModeSwitches: Invalid parameters (coinNode={valid|null}, m_useSwitchMode={true|false})
MeshDisplayModeHandler::updateDisplayModeSwitches: Switches not initialized (surface={valid|null}, edges={valid|null}, points={valid|null})
```

#### Configuration Loading
```
MeshDisplayModeHandler::updateDisplayModeSwitches: Config loaded - requireSurface={true|false}, requireMeshEdges={true|false}, requireOriginalEdges={true|false}, requirePoints={true|false}, meshEdge.enabled={true|false}
```

#### Wireframe Mode Special Case
```
MeshDisplayModeHandler::updateDisplayModeSwitches: Wireframe mode - forcing showMeshEdges=true
```

#### Switch Visibility Updates
```
MeshDisplayModeHandler::updateDisplayModeSwitches: Surface switch: {oldValue} -> {newValue} (requireSurface={true|false})
MeshDisplayModeHandler::updateDisplayModeSwitches: Edges switch: {oldValue} -> {newValue} (requireMeshEdges={true|false}, requireOriginalEdges={true|false}, meshEdge.enabled={true|false}, showMeshEdges={true|false}, showEdges={true|false})
MeshDisplayModeHandler::updateDisplayModeSwitches: Points switch: {oldValue} -> {newValue} (requirePoints={true|false})
MeshDisplayModeHandler::updateDisplayModeSwitches: Switch update completed successfully
```

**When Logged**:
- When updating switch visibility for mesh geometries
- Similar to BREP handler but includes mesh-specific logic (requireMeshEdges)

---

### 4. BRepDisplayModeHandler::updateSwitchVisibility()

**File**: `wxcoin/src/opencascade/geometry/helper/BRepDisplayModeHandler.cpp`

**Log Messages**:

#### Initialization Check
```
BRepDisplayModeHandler::updateSwitchVisibility: Updating switch visibility - requireSurface={true|false}, requireOriginalEdges={true|false}, requireMeshEdges={true|false}, requirePoints={true|false}
```

#### Error Cases
```
BRepDisplayModeHandler::updateSwitchVisibility: Switches not initialized
```

#### Edge Display Logic
```
BRepDisplayModeHandler::updateSwitchVisibility: Edge display logic - showOriginalEdges={true|false}, showMeshEdges={true|false}, useModularEdgeComponent={true|false}, edgeComponent={valid|null}
BRepDisplayModeHandler::updateSwitchVisibility: Cleared edges node
BRepDisplayModeHandler::updateSwitchVisibility: Extracting original edges
BRepDisplayModeHandler::updateSwitchVisibility: Applying appearance to existing edges
BRepDisplayModeHandler::updateSwitchVisibility: Updated edge display in edges node
```

#### Switch Visibility Updates
```
BRepDisplayModeHandler::updateSwitchVisibility: Surface switch: {oldValue} -> {newValue}
BRepDisplayModeHandler::updateSwitchVisibility: Edges switch: {oldValue} -> {newValue}
BRepDisplayModeHandler::updateSwitchVisibility: Points switch: {oldValue} -> {newValue}
BRepDisplayModeHandler::updateSwitchVisibility: Switch visibility update completed
```

**When Logged**:
- Called from `handleDisplayMode()` when initializing or updating switch structure
- Shows detailed edge extraction and appearance application

---

### 5. MeshDisplayModeHandler::updateSwitchVisibility()

**File**: `wxcoin/src/opencascade/geometry/helper/MeshDisplayModeHandler.cpp`

**Log Messages**:

#### Initialization Check
```
MeshDisplayModeHandler::updateSwitchVisibility: Updating switch visibility for displayMode={modeIndex} - requireSurface={true|false}, requireMeshEdges={true|false}, requireOriginalEdges={true|false}, requirePoints={true|false}
```

#### Wireframe Mode Special Case
```
MeshDisplayModeHandler::updateSwitchVisibility: Wireframe mode - forcing showMeshEdges=true
```

#### Error Cases
```
MeshDisplayModeHandler::updateSwitchVisibility: Switches not initialized
```

#### Edge Display Logic
```
MeshDisplayModeHandler::updateSwitchVisibility: Edge display logic - showMeshEdges={true|false}, meshEdge.enabled={true|false}, showEdges={true|false}, useModularEdgeComponent={true|false}, edgeComponent={valid|null}
MeshDisplayModeHandler::updateSwitchVisibility: Cleared edges node
MeshDisplayModeHandler::updateSwitchVisibility: Extracting mesh edges
MeshDisplayModeHandler::updateSwitchVisibility: Applied effective color (black)
MeshDisplayModeHandler::updateSwitchVisibility: Using original edge color for Wireframe/NoShading
MeshDisplayModeHandler::updateSwitchVisibility: Applying appearance to existing mesh edges
MeshDisplayModeHandler::updateSwitchVisibility: Clearing silhouette edge node for HiddenLine mode
MeshDisplayModeHandler::updateSwitchVisibility: Updated edge display in edges node
```

#### Switch Visibility Updates
```
MeshDisplayModeHandler::updateSwitchVisibility: Surface switch: {oldValue} -> {newValue}
MeshDisplayModeHandler::updateSwitchVisibility: Edges switch: {oldValue} -> {newValue}
MeshDisplayModeHandler::updateSwitchVisibility: Points switch: {oldValue} -> {newValue}
MeshDisplayModeHandler::updateSwitchVisibility: Switch visibility update completed
```

**When Logged**:
- Called from `handleDisplayMode()` when initializing or updating switch structure
- Shows mesh-specific edge handling (effective color, silhouette clearing)

---

## Switch Value Meanings

### whichChild Values
- **0**: Show (child[0] is visible)
- **-1**: Hide (no child visible)

### Example Log Output

```
DisplayModeHandler::updateDisplayMode: Switching to mode=3 using Switch mode
DisplayModeHandler::updateDisplayMode: Calling BREP handler for switch update
BRepDisplayModeHandler::updateDisplayModeSwitches: Starting switch update for mode=3
BRepDisplayModeHandler::updateDisplayModeSwitches: Found 3 switch nodes in coinNode
BRepDisplayModeHandler::updateDisplayModeSwitches: Config loaded - requireSurface=true, requireOriginalEdges=true, requirePoints=false, originalEdge.enabled=true
BRepDisplayModeHandler::updateDisplayModeSwitches: Surface switch: -1 -> 0 (requireSurface=true)
BRepDisplayModeHandler::updateDisplayModeSwitches: Edges switch: -1 -> 0 (requireOriginalEdges=true, originalEdge.enabled=true, showOriginalEdges=true)
BRepDisplayModeHandler::updateDisplayModeSwitches: Points switch: 0 -> -1 (requirePoints=false)
BRepDisplayModeHandler::updateDisplayModeSwitches: Switch update completed successfully
```

---

## Logging Levels

All logging uses `LOG_INF_S()` (Info level) for normal operation and `LOG_WRN_S()` (Warning level) for error conditions.

### Info Level (`LOG_INF_S`)
- Normal switch updates
- Configuration loading
- Switch value changes
- Edge extraction/appearance updates

### Warning Level (`LOG_WRN_S`)
- Invalid parameters
- Switches not initialized
- Missing required components

---

## Usage

### Enabling Logging

Logging is always enabled. To view logs:

1. **Console Output**: Logs appear in the application console
2. **Log File**: Check application log file (location depends on Logger configuration)
3. **Debug Output**: Use debugger to view log messages

### Filtering Logs

To filter display mode switch logs, search for:
- `DisplayModeHandler::updateDisplayMode`
- `BRepDisplayModeHandler::updateDisplayModeSwitches`
- `MeshDisplayModeHandler::updateDisplayModeSwitches`
- `updateSwitchVisibility`

---

## Debugging Tips

1. **Switch Not Updating**: Check for "Switches not initialized" warnings
2. **Wrong Visibility**: Compare old/new switch values in logs
3. **Configuration Issues**: Verify config values match expected behavior
4. **Edge Display Problems**: Check edge extraction and appearance logs
5. **Mode Detection**: Verify correct handler (BREP vs Mesh) is called

---

## Summary

Logging provides comprehensive visibility into:
- **Mode switching**: Which mode is being switched to
- **Switch updates**: Old and new visibility values
- **Configuration**: Values that determine visibility
- **Edge handling**: Extraction and appearance updates
- **Error conditions**: Missing or invalid components

This makes it easy to debug display mode switching issues and verify correct behavior.







