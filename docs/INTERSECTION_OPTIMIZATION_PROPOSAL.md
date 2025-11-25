# Intersection Computation Optimization Proposal

## Current Status Analysis

### 1. Current Algorithm
- **Spatial Grid Partitioning**: Uses 3D grid to filter candidate edge pairs (O(n·k) where k is edges per cell)
- **Sampling Method**: Uses 10-16 sample points per edge pair for distance-based intersection detection
- **Two-stage Detection**: Coarse check (6 samples) + Fine check (8 samples)
- **Problem**: Sampling method is inefficient - many unnecessary distance calculations

### 2. Current Cache Mechanism
- **EdgeGeometryCache**: Caches intersection points as a simple vector
- **Missing**: No relationship between edges and intersection points
- **Missing**: Cannot do incremental updates when geometry changes

### 3. BVH Accelerator Status
- **EdgeIntersectionAccelerator**: Already implemented using BVH
- **Problem**: Not fully integrated into OriginalEdgeExtractor
- **Potential**: Can reduce O(n²) to O(n log n) for large models

## Proposed Improvements

### Improvement 1: Enhanced Cache with Edge-Intersection Relationships

**Goal**: Store intersection points with their associated edge pairs for incremental updates

**New Cache Structure**:
```cpp
struct IntersectionCacheEntry {
    // Existing fields
    std::vector<gp_Pnt> intersectionPoints;
    size_t shapeHash;
    double tolerance;
    
    // NEW: Edge-intersection relationships
    struct EdgeIntersection {
        size_t edge1Index;      // Index in edge list
        size_t edge2Index;      // Index in edge list
        gp_Pnt intersectionPoint;
        double distance;         // Distance between edges at intersection
        
        EdgeIntersection(size_t i1, size_t i2, const gp_Pnt& pt, double dist)
            : edge1Index(i1), edge2Index(i2), intersectionPoint(pt), distance(dist) {}
    };
    
    std::vector<EdgeIntersection> edgeIntersections;  // NEW: Relationships
    
    // Edge list for reference
    std::vector<TopoDS_Edge> edges;  // NEW: Store edges for validation
    std::vector<size_t> edgeHashes;  // NEW: Hash of each edge for change detection
};
```

**Benefits**:
- Can identify which edges changed
- Can update only affected intersections
- Can validate cache by checking edge hashes

### Improvement 2: Use BVH Instead of Spatial Grid

**Current**: Spatial grid with fixed cell size (max 32x32x32)
**Proposed**: Use EdgeIntersectionAccelerator (BVH) for models with >= 100 edges

**Implementation**:
```cpp
void OriginalEdgeExtractor::findEdgeIntersectionsFromEdges(
    const std::vector<TopoDS_Edge>& edges,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {
    
    // Use BVH for large models
    if (edges.size() >= 100) {
        EdgeIntersectionAccelerator accelerator;
        accelerator.buildFromEdges(edges);
        auto pairs = accelerator.findPotentialIntersections();
        
        // Compute intersections for candidate pairs only
        std::vector<gp_Pnt> points;
        for (const auto& pair : pairs) {
            gp_Pnt intersection;
            if (accelerator.computeEdgeIntersection(
                    accelerator.getEdgePrimitive(pair.edge1Index),
                    accelerator.getEdgePrimitive(pair.edge2Index),
                    tolerance, intersection)) {
                points.push_back(intersection);
            }
        }
        intersectionPoints = std::move(points);
        return;
    }
    
    // Fall back to spatial grid for small models
    // ... existing spatial grid code ...
}
```

**Benefits**:
- Better pruning efficiency (90%+ vs 60-70% for grid)
- Adaptive to edge distribution (grid is fixed size)
- O(n log n) vs O(n·k) complexity

### Improvement 3: Incremental Update Mechanism

**Goal**: When geometry changes, only recompute affected intersections

**Implementation**:
```cpp
class EdgeGeometryCache {
public:
    struct IncrementalUpdateResult {
        std::vector<gp_Pnt> validIntersections;  // Still valid
        std::vector<gp_Pnt> newIntersections;   // Need to compute
        std::vector<size_t> invalidatedEdgeIndices;  // Edges that changed
    };
    
    IncrementalUpdateResult updateIntersectionsIncremental(
        const std::string& key,
        const std::vector<TopoDS_Edge>& currentEdges,
        double tolerance) {
        
        auto it = m_intersectionCache.find(key);
        if (it == m_intersectionCache.end()) {
            // Cache miss - full computation
            return computeAll();
        }
        
        auto& entry = it->second;
        IncrementalUpdateResult result;
        
        // Check which edges changed
        std::vector<bool> edgeChanged(currentEdges.size(), true);
        for (size_t i = 0; i < currentEdges.size() && i < entry.edgeHashes.size(); ++i) {
            size_t currentHash = computeEdgeHash(currentEdges[i]);
            if (currentHash == entry.edgeHashes[i]) {
                edgeChanged[i] = false;
            }
        }
        
        // Keep valid intersections (both edges unchanged)
        for (const auto& ei : entry.edgeIntersections) {
            if (!edgeChanged[ei.edge1Index] && !edgeChanged[ei.edge2Index]) {
                result.validIntersections.push_back(ei.intersectionPoint);
            }
        }
        
        // Find edges that need recomputation
        for (size_t i = 0; i < currentEdges.size(); ++i) {
            if (edgeChanged[i]) {
                result.invalidatedEdgeIndices.push_back(i);
            }
        }
        
        // Compute new intersections only for changed edges
        result.newIntersections = computeIntersectionsForEdges(
            result.invalidatedEdgeIndices, currentEdges, tolerance);
        
        return result;
    }
};
```

**Benefits**:
- Only recompute intersections involving changed edges
- Significant speedup when only small parts of geometry change
- Maintains cache validity

### Improvement 4: Use OpenCASCADE's Native Intersection Methods

**Current**: Sampling-based distance detection (inefficient)
**Proposed**: Use GeomAPI_ExtremaCurveCurve for accurate intersection

**Implementation**:
```cpp
bool computeEdgeIntersection(
    const TopoDS_Edge& edge1,
    const TopoDS_Edge& edge2,
    double tolerance,
    gp_Pnt& intersection) {
    
    Standard_Real first1, last1, first2, last2;
    Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edge1, first1, last1);
    Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edge2, first2, last2);
    
    if (curve1.IsNull() || curve2.IsNull()) return false;
    
    // Use OpenCASCADE's native extrema algorithm
    GeomAPI_ExtremaCurveCurve extrema(curve1, curve2, first1, last1, first2, last2);
    
    if (extrema.NbExtrema() > 0) {
        double minDist = std::numeric_limits<double>::max();
        int minIndex = -1;
        
        for (int i = 1; i <= extrema.NbExtrema(); ++i) {
            double dist = extrema.Distance(i);
            if (dist < minDist) {
                minDist = dist;
                minIndex = i;
            }
        }
        
        if (minIndex > 0 && minDist < tolerance) {
            gp_Pnt p1, p2;
            extrema.Points(minIndex, p1, p2);
            intersection = gp_Pnt((p1.X() + p2.X()) / 2.0,
                                 (p1.Y() + p2.Y()) / 2.0,
                                 (p1.Z() + p2.Z()) / 2.0);
            return true;
        }
    }
    
    return false;
}
```

**Benefits**:
- More accurate than sampling
- Faster for most cases (native optimized code)
- Handles all curve types properly

## Implementation Plan

### Phase 1: Enhanced Cache Structure (Week 1)
1. Extend `IntersectionCacheEntry` with edge-intersection relationships
2. Update cache storage to include edge hashes
3. Add validation methods

### Phase 2: BVH Integration (Week 2)
1. Integrate `EdgeIntersectionAccelerator` into `OriginalEdgeExtractor`
2. Add threshold-based selection (BVH for >= 100 edges)
3. Performance testing and tuning

### Phase 3: Native Intersection Methods (Week 2)
1. Replace sampling with `GeomAPI_ExtremaCurveCurve`
2. Update `EdgeIntersectionAccelerator::computeEdgeIntersection`
3. Accuracy and performance validation

### Phase 4: Incremental Updates (Week 3)
1. Implement `updateIntersectionsIncremental` method
2. Add edge change detection
3. Integration with display update logic

## Expected Performance Improvements

| Scenario | Current | After Optimization | Improvement |
|----------|---------|-------------------|-------------|
| Large model (1000+ edges) | 10-30s | 2-5s | 5-6x faster |
| Small change (10% edges) | 10-30s | 0.5-1s | 20-30x faster |
| Cache hit | 0s | 0s | Same |
| Memory overhead | Low | Medium (+20%) | Acceptable |

## Summary

**Key Improvements**:
1. ✅ **Spatial indexing**: Use BVH instead of fixed grid
2. ✅ **Edge-intersection relationships**: Store which edges produce which intersections
3. ✅ **Caching with relationships**: Enable incremental updates
4. ✅ **Native methods**: Use OpenCASCADE's optimized intersection algorithms

**Result**: Faster computation, better cache utilization, incremental updates for changed geometry

