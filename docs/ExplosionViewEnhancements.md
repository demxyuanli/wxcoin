# Explosion View Enhancements

## Overview
The explosion view feature has been enhanced with advanced algorithms based on constraint-driven analysis, direction clustering, and collision detection.

## New Features

### 1. Smart Mode
A new "Smart" explosion mode that automatically determines the best explosion direction based on:
- Assembly constraint analysis (if available)
- Geometric distribution of parts
- K-Means clustering of constraint directions

**How it works:**
1. Collects all constraint directions from the assembly
2. Uses K-Means clustering (K=3) to find the dominant direction
3. Applies a blend of the clustered direction and radial component (70/30)
4. Falls back to geometric analysis if no constraints are available

### 2. Direction Clustering Algorithm
Implements K-Means clustering without external dependencies (Eigen), using only OpenCASCADE:
- Groups similar constraint directions
- Identifies the main explosion direction automatically
- Handles bidirectional constraints

**Algorithm details:**
- Uses 3 clusters to capture primary, secondary, and tertiary directions
- 20 iterations by default (configurable)
- Selects the largest cluster as the main direction

### 3. Collision Detection and Resolution
Optional collision detection system that prevents parts from overlapping after explosion:
- Pairwise bounding box collision detection
- Multi-pass resolution (3 passes by default)
- Pushes colliding parts along the main explosion direction

**Parameters:**
- `enableCollisionResolution`: Toggle collision detection on/off
- `collisionThreshold`: Controls the minimum spacing (0.0-1.0, default 0.6)

### 4. Assembly Constraints Support
New data structures for assembly relationships:
- `ConstraintType`: MATE, INSERT, FASTENER, UNKNOWN
- `AssemblyConstraint`: Stores relationships between parts with separation directions
- Can be populated from CAD assembly metadata

## Enhanced Parameters

### Existing Parameters (Improved)
- **baseFactor**: Global distance multiplier (0.01-10.0)
- **perLevelScale**: Hierarchy-based scaling for Assembly mode
- **sizeInfluence**: Scale explosion based on part size
- **jitter**: Random perturbation for visual variety
- **minSpacing**: Minimum movement threshold

### New Parameters
- **enableCollisionResolution**: Enable/disable collision detection
- **collisionThreshold**: Collision sensitivity (fraction of bbox diagonal)
- **constraints**: Vector of AssemblyConstraint for Smart mode

## Usage

### Basic Usage (UI)
1. Open an assembly model with multiple parts
2. Click "Explode Assembly" command
3. Select explosion mode:
   - **Radial**: Traditional radial explosion
   - **Axis X/Y/Z**: Explosion along specific axis
   - **Stack X/Y/Z**: Directional stacking
   - **Diagonal**: 45-degree diagonal explosion
   - **Assembly**: Hierarchy-based explosion
   - **Smart**: AI-driven direction analysis
4. Adjust distance factor
5. Enable collision resolution if needed
6. Click OK

### Programmatic Usage

```cpp
// Create explode parameters
ExplodeParams params;
params.primaryMode = ExplodeMode::Smart;
params.baseFactor = 1.5;
params.enableCollisionResolution = true;
params.collisionThreshold = 0.6;

// Add constraints (optional)
params.constraints.push_back(
    AssemblyConstraint("part1", "part2", 
                       ConstraintType::MATE, 
                       gp_Dir(0, 0, 1))
);

// Apply to viewer
viewer->setExplodeParamsAdvanced(params);
viewer->setExplodeEnabled(true, params.baseFactor);
```

## Algorithm Details

### K-Means Direction Clustering
```
Input: Vector of gp_Dir (constraint directions)
Output: Dominant direction (gp_Dir)

1. Initialize 3 random cluster centers from input directions
2. For maxIterations:
   a. Assign each direction to nearest cluster center
   b. Recalculate cluster centers as normalized mean of assigned directions
3. Return center of largest cluster
```

### Collision Resolution
```
Input: Vector of offsets, geometries, main direction
Output: Modified offsets (by reference)

1. Compute new positions after applying offsets
2. For maxPasses:
   a. For each pair of parts:
      - Calculate distance between centers
      - If distance < minDistance:
        * Push parts apart along main direction
        * Proportional to overlap amount
   b. If no collisions detected, exit early
```

### Smart Direction Analysis
```
Input: Vector of geometries, constraint parameters
Output: Main explosion direction (gp_Dir)

1. If constraints exist:
   a. Collect all constraint directions
   b. Add opposite directions (bidirectional)
   c. Apply K-Means clustering
   d. Return dominant direction
2. If no constraints:
   a. Compute global bounding box
   b. Find longest axis
   c. Return direction along longest axis
```

## Performance Considerations

1. **Direction Clustering**: O(N*K*I) where N=directions, K=3, I=iterations
2. **Collision Detection**: O(P^2*M) where P=parts, M=maxPasses (3)
3. **Overall**: Scales linearly with number of parts for typical assemblies

## Future Enhancements

Potential areas for further improvement:
1. Hierarchical clustering for complex assemblies
2. OBB (Oriented Bounding Box) collision detection
3. Animation path planning
4. Assembly sequence optimization
5. Constraint extraction from STEP/IGES files
6. Interactive constraint editing
7. GPU acceleration for large assemblies

## API Changes

### ExplodeTypes.h
- Added `ExplodeMode::Smart`
- Added `ConstraintType` enum
- Added `AssemblyConstraint` struct
- Added collision parameters to `ExplodeParams`

### ExplodeController.h
- Added `clusterDirections()` method
- Added `analyzeConstraintsDirection()` method
- Added `resolveCollisions()` method
- Added `getBBoxDiagonal()` helper

### ExplodeConfigDialog
- Added "Smart" mode option
- Added collision detection checkbox
- Added collision threshold slider
- Updated mode selection logic

## Testing Recommendations

1. Test with simple assemblies (2-5 parts)
2. Test with complex assemblies (50+ parts)
3. Test with hierarchical assemblies
4. Test collision resolution with tightly packed parts
5. Verify Smart mode fallback when no constraints
6. Performance testing with 100+ parts

## Known Limitations

1. Smart mode requires constraint data (falls back to geometric analysis)
2. Collision detection uses simple sphere-based approximation
3. No support for non-rigid transformations
4. Constraints must be pre-defined (no automatic extraction)

## References

- Original algorithm inspiration: ExplosionView.cpp example
- K-Means clustering: Lloyd's algorithm
- Collision detection: Axis-Aligned Bounding Box (AABB) method










