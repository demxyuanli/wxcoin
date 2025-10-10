# Quick Optimization Implementation Guide

## Quick Reference: Code Changes by Module

---

## 1. Edge Geometry Caching (HIGH PRIORITY - 1 week)

### File: `include/edges/EdgeGeometryCache.h` (NEW)

```cpp
#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>

class EdgeGeometryCache {
public:
    struct CacheEntry {
        std::vector<gp_Pnt> points;
        size_t shapeHash;
        std::chrono::steady_clock::time_point lastAccess;
    };

    static EdgeGeometryCache& getInstance() {
        static EdgeGeometryCache instance;
        return instance;
    }

    std::vector<gp_Pnt> getOrCompute(
        const std::string& key,
        std::function<std::vector<gp_Pnt>()> computeFunc);

    void invalidate(const std::string& key);
    void clear();
    void evictOldEntries(std::chrono::seconds maxAge = std::chrono::seconds(300));

    size_t getCacheSize() const { return m_cache.size(); }
    size_t getHitCount() const { return m_hitCount; }
    size_t getMissCount() const { return m_missCount; }

private:
    EdgeGeometryCache() = default;
    std::unordered_map<std::string, CacheEntry> m_cache;
    std::mutex m_mutex;
    size_t m_hitCount{0};
    size_t m_missCount{0};
};
```

### File: `src/opencascade/edges/EdgeGeometryCache.cpp` (NEW)

```cpp
#include "edges/EdgeGeometryCache.h"
#include "logger/Logger.h"

std::vector<gp_Pnt> EdgeGeometryCache::getOrCompute(
    const std::string& key,
    std::function<std::vector<gp_Pnt>()> computeFunc)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Cache hit
        it->second.lastAccess = std::chrono::steady_clock::now();
        m_hitCount++;
        
        LOG_DBG_S("Edge cache HIT: " + key + 
                  " (hit rate: " + std::to_string(100.0 * m_hitCount / (m_hitCount + m_missCount)) + "%)");
        
        return it->second.points;
    }

    // Cache miss - compute
    m_missCount++;
    auto points = computeFunc();
    
    CacheEntry entry;
    entry.points = points;
    entry.shapeHash = 0; // TODO: compute actual hash
    entry.lastAccess = std::chrono::steady_clock::now();
    
    m_cache[key] = std::move(entry);
    
    LOG_DBG_S("Edge cache MISS: " + key + " (computed " + std::to_string(points.size()) + " points)");
    
    return points;
}

void EdgeGeometryCache::invalidate(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.erase(key);
}

void EdgeGeometryCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_hitCount = 0;
    m_missCount = 0;
}

void EdgeGeometryCache::evictOldEntries(std::chrono::seconds maxAge) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (now - it->second.lastAccess > maxAge) {
            LOG_DBG_S("Evicting old cache entry: " + it->first);
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}
```

### Modification: `src/opencascade/edges/EdgeExtractor.cpp`

```cpp
// Add at top of file
#include "edges/EdgeGeometryCache.h"

// Modify extractOriginalEdges method
std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdges(
    const TopoDS_Shape& shape, 
    double samplingDensity, 
    double minLength, 
    bool showLinesOnly,
    std::vector<gp_Pnt>* intersectionPoints)
{
    // Generate cache key
    std::string cacheKey = "original_" + 
                           std::to_string(shape.HashCode(INT_MAX)) + "_" +
                           std::to_string(samplingDensity) + "_" +
                           std::to_string(minLength) + "_" +
                           std::to_string(showLinesOnly);
    
    // Use cache
    auto& cache = EdgeGeometryCache::getInstance();
    return cache.getOrCompute(cacheKey, [&]() {
        // Original extraction code here...
        return originalExtractionLogic(shape, samplingDensity, minLength, showLinesOnly);
    });
}
```

**Expected Result**: 80-90% faster edge display toggling after first computation.

---

## 2. Intelligent Mesh Parameter Advisor (HIGH PRIORITY - 2 weeks)

### File: `include/viewer/MeshParameterAdvisor.h` (NEW)

```cpp
#pragma once

#include "rendering/GeometryProcessor.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>

struct ShapeComplexity {
    double avgCurvature;
    size_t faceCount;
    size_t edgeCount;
    double boundingBoxSize;
    double surfaceArea;
    bool hasComplexSurfaces;
};

class MeshParameterAdvisor {
public:
    static MeshParameters recommendParameters(const TopoDS_Shape& shape);
    static ShapeComplexity analyzeShape(const TopoDS_Shape& shape);
    static size_t estimateTriangleCount(const TopoDS_Shape& shape, const MeshParameters& params);
    
    // Quality presets
    static MeshParameters getDraftPreset(const TopoDS_Shape& shape);
    static MeshParameters getLowPreset(const TopoDS_Shape& shape);
    static MeshParameters getMediumPreset(const TopoDS_Shape& shape);
    static MeshParameters getHighPreset(const TopoDS_Shape& shape);
    static MeshParameters getVeryHighPreset(const TopoDS_Shape& shape);
};
```

### File: `src/viewer/MeshParameterAdvisor.cpp` (NEW)

```cpp
#include "viewer/MeshParameterAdvisor.h"
#include "logger/Logger.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

ShapeComplexity MeshParameterAdvisor::analyzeShape(const TopoDS_Shape& shape) {
    ShapeComplexity complexity;
    
    // Calculate bounding box
    Bnd_Box bbox;
    BRepBndLib::Add(shape, bbox);
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    
    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;
    complexity.boundingBoxSize = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Count faces and edges
    complexity.faceCount = 0;
    complexity.edgeCount = 0;
    
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        complexity.faceCount++;
    }
    
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        complexity.edgeCount++;
    }
    
    // Calculate surface area
    GProp_GProps props;
    BRepGProp::SurfaceProperties(shape, props);
    complexity.surfaceArea = props.Mass();
    
    // Check for complex surfaces
    complexity.hasComplexSurfaces = false;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        BRepAdaptor_Surface surf(face);
        GeomAbs_SurfaceType type = surf.GetType();
        
        if (type == GeomAbs_BSplineSurface || type == GeomAbs_BezierSurface) {
            complexity.hasComplexSurfaces = true;
            break;
        }
    }
    
    // Estimate average curvature (simplified)
    if (complexity.surfaceArea > 0 && complexity.faceCount > 0) {
        complexity.avgCurvature = complexity.faceCount / complexity.surfaceArea;
    } else {
        complexity.avgCurvature = 0.0;
    }
    
    return complexity;
}

MeshParameters MeshParameterAdvisor::recommendParameters(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.relative = false;
    params.inParallel = (complexity.faceCount > 100);
    
    // Size-based deflection
    if (complexity.boundingBoxSize < 10.0) {
        params.deflection = 0.001;  // Very fine for small parts
    } else if (complexity.boundingBoxSize < 100.0) {
        params.deflection = 0.01;   // Medium for typical parts
    } else if (complexity.boundingBoxSize < 1000.0) {
        params.deflection = 0.1;    // Coarse for large parts
    } else {
        params.deflection = 1.0;    // Very coarse for huge assemblies
    }
    
    // Complexity adjustment
    if (complexity.hasComplexSurfaces) {
        params.deflection *= 0.5;  // Finer for complex surfaces
    }
    
    if (complexity.avgCurvature > 0.1) {
        params.deflection *= 0.7;  // Finer for high curvature
    }
    
    // Angular deflection (proportional to linear deflection)
    params.angularDeflection = std::min(0.5, params.deflection * 10.0);
    
    LOG_INF_S("Mesh parameter recommendation:");
    LOG_INF_S("  BBox size: " + std::to_string(complexity.boundingBoxSize));
    LOG_INF_S("  Faces: " + std::to_string(complexity.faceCount));
    LOG_INF_S("  Edges: " + std::to_string(complexity.edgeCount));
    LOG_INF_S("  Deflection: " + std::to_string(params.deflection));
    LOG_INF_S("  Angular: " + std::to_string(params.angularDeflection));
    
    return params;
}

size_t MeshParameterAdvisor::estimateTriangleCount(
    const TopoDS_Shape& shape, 
    const MeshParameters& params)
{
    ShapeComplexity complexity = analyzeShape(shape);
    
    // Rough estimation: surface area / average triangle area
    // Triangle area ≈ deflection²
    double avgTriangleArea = params.deflection * params.deflection;
    size_t estimate = static_cast<size_t>(complexity.surfaceArea / avgTriangleArea * 2.0);
    
    // Apply complexity factor
    if (complexity.hasComplexSurfaces) {
        estimate = static_cast<size_t>(estimate * 1.5);
    }
    
    LOG_INF_S("Estimated triangle count: ~" + std::to_string(estimate));
    
    return estimate;
}

MeshParameters MeshParameterAdvisor::getMediumPreset(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.deflection = complexity.boundingBoxSize * 0.01;  // 1% of model size
    params.angularDeflection = 0.1;
    params.relative = false;
    params.inParallel = (complexity.faceCount > 100);
    
    return params;
}

// Similar implementations for other presets...
```

### Integration: Modify `src/ui/MeshQualityDialog.cpp`

```cpp
// Add button to UI
void MeshQualityDialog::OnAutoRecommend(wxCommandEvent& event) {
    if (!m_currentShape.IsNull()) {
        auto params = MeshParameterAdvisor::recommendParameters(m_currentShape);
        
        // Update UI controls
        m_deflectionCtrl->SetValue(wxString::Format("%.6f", params.deflection));
        m_angularDeflectionCtrl->SetValue(wxString::Format("%.6f", params.angularDeflection));
        m_parallelCheckbox->SetValue(params.inParallel);
        
        // Show estimate
        size_t estimatedTris = MeshParameterAdvisor::estimateTriangleCount(m_currentShape, params);
        m_estimateLabel->SetLabel(wxString::Format("Estimated triangles: ~%zu", estimatedTris));
        
        wxMessageBox("Parameters automatically adjusted for this geometry", "Auto Recommend");
    }
}
```

**Expected Result**: Better default quality, 50-80% reduction in user parameter adjustments.

---

## 3. Adaptive Edge Sampling (HIGH PRIORITY - 2 weeks)

### Modification: `src/opencascade/edges/EdgeExtractor.cpp`

```cpp
// Add new method for adaptive sampling
std::vector<gp_Pnt> EdgeExtractor::adaptiveSampleCurve(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType)
{
    std::vector<gp_Pnt> points;
    
    // Fast path for lines
    if (curveType == GeomAbs_Line) {
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
        return points;
    }
    
    // Analyze curvature
    const int analysisPoints = 10;
    double maxCurvature = 0.0;
    
    for (int i = 0; i <= analysisPoints; ++i) {
        Standard_Real t = first + (last - first) * i / analysisPoints;
        
        gp_Pnt p;
        gp_Vec d1, d2;
        curve->D2(t, p, d1, d2);
        
        double curvature = d1.Crossed(d2).Magnitude() / std::pow(d1.Magnitude(), 3);
        maxCurvature = std::max(maxCurvature, curvature);
    }
    
    // Determine sample count based on curvature
    int numSamples;
    if (maxCurvature < 0.01) {
        numSamples = 4;      // Low curvature
    } else if (maxCurvature < 0.1) {
        numSamples = 8;      // Medium curvature
    } else if (maxCurvature < 1.0) {
        numSamples = 16;     // High curvature
    } else {
        numSamples = 32;     // Very high curvature
    }
    
    // Additional samples for specific curve types
    switch (curveType) {
        case GeomAbs_Circle:
        case GeomAbs_Ellipse:
            numSamples = std::max(numSamples, 16);
            break;
        case GeomAbs_BSplineCurve:
        case GeomAbs_BezierCurve:
            numSamples = std::max(numSamples, 20);
            break;
        default:
            break;
    }
    
    // Cap maximum
    numSamples = std::min(numSamples, 64);
    
    // Generate points
    points.reserve(numSamples + 1);
    for (int i = 0; i <= numSamples; ++i) {
        Standard_Real t = first + (last - first) * i / numSamples;
        points.push_back(curve->Value(t));
    }
    
    LOG_DBG_S("Adaptive sampling: curvature=" + std::to_string(maxCurvature) + 
              ", samples=" + std::to_string(numSamples));
    
    return points;
}

// Modify extractOriginalEdges to use adaptive sampling
std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdges(...) {
    // ... existing code ...
    
    // Replace fixed sampling with adaptive
    data.sampledPoints = adaptiveSampleCurve(
        data.curve, data.first, data.last, data.curveType);
    
    // ... rest of code ...
}
```

**Expected Result**: 40-60% reduction in edge points for simple geometry, better quality for complex curves.

---

## 4. Quick Integration Checklist

### Immediate Actions (Week 1)

- [ ] Create `EdgeGeometryCache` class
- [ ] Integrate cache into `EdgeExtractor`
- [ ] Add cache statistics logging
- [ ] Test with sample models

### Week 2-3 Actions

- [ ] Create `MeshParameterAdvisor` class
- [ ] Add "Auto Recommend" button to UI
- [ ] Implement triangle count estimation
- [ ] Test with various model sizes

### Week 3-4 Actions

- [ ] Implement adaptive edge sampling
- [ ] Add curvature analysis
- [ ] Benchmark performance improvements
- [ ] Document parameter recommendations

---

## 5. Performance Testing Script

### File: `test/performance/TestOptimizations.cpp` (NEW)

```cpp
#include <chrono>
#include <iostream>
#include "edges/EdgeExtractor.h"
#include "edges/EdgeGeometryCache.h"
#include "viewer/MeshParameterAdvisor.h"

void testEdgeCaching() {
    EdgeExtractor extractor;
    TopoDS_Shape testShape = loadTestModel("test_model.step");
    
    auto& cache = EdgeGeometryCache::getInstance();
    cache.clear();
    
    // First call (cache miss)
    auto start1 = std::chrono::high_resolution_clock::now();
    auto edges1 = extractor.extractOriginalEdges(testShape, 80.0, 0.01, false);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    
    // Second call (cache hit)
    auto start2 = std::chrono::high_resolution_clock::now();
    auto edges2 = extractor.extractOriginalEdges(testShape, 80.0, 0.01, false);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "First call (cache miss): " << duration1.count() << "ms\n";
    std::cout << "Second call (cache hit): " << duration2.count() << "ms\n";
    std::cout << "Speedup: " << (double)duration1.count() / duration2.count() << "x\n";
    std::cout << "Cache hit rate: " << 
        (100.0 * cache.getHitCount() / (cache.getHitCount() + cache.getMissCount())) << "%\n";
}

void testParameterRecommendation() {
    TopoDS_Shape smallPart = loadTestModel("small_part.step");
    TopoDS_Shape largePart = loadTestModel("large_assembly.step");
    
    auto params1 = MeshParameterAdvisor::recommendParameters(smallPart);
    auto params2 = MeshParameterAdvisor::recommendParameters(largePart);
    
    std::cout << "Small part deflection: " << params1.deflection << "\n";
    std::cout << "Large part deflection: " << params2.deflection << "\n";
    
    size_t est1 = MeshParameterAdvisor::estimateTriangleCount(smallPart, params1);
    size_t est2 = MeshParameterAdvisor::estimateTriangleCount(largePart, params2);
    
    std::cout << "Small part estimated triangles: " << est1 << "\n";
    std::cout << "Large part estimated triangles: " << est2 << "\n";
}

int main() {
    std::cout << "=== Edge Caching Test ===\n";
    testEdgeCaching();
    
    std::cout << "\n=== Parameter Recommendation Test ===\n";
    testParameterRecommendation();
    
    return 0;
}
```

---

## 6. CMakeLists.txt Modifications

### Add to `src/opencascade/CMakeLists.txt`

```cmake
# Add new cache implementation
target_sources(opencascade PRIVATE
    edges/EdgeGeometryCache.cpp
)

# Add new advisor implementation
target_sources(opencascade PRIVATE
    viewer/MeshParameterAdvisor.cpp
)
```

---

## 7. Configuration Parameters

### Add to appropriate config file

```ini
[EdgeOptimization]
EnableGeometryCache=true
CacheMaxAge=300
CacheMaxEntries=100

[MeshOptimization]
EnableAutoRecommend=true
DefaultQualityPreset=Medium

[PerformanceMonitoring]
LogCacheStatistics=true
LogParameterRecommendations=true
```

---

## Expected Overall Impact After Phase 1

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Edge display toggle | 200ms | 20ms | 10x faster |
| Parameter adjustment time | 5 trials avg | 1-2 trials | 60-80% reduction |
| Edge point count (simple) | 100% | 40-60% | Significant reduction |
| User satisfaction | Baseline | High | Better defaults |

---

## Next Steps

After completing Phase 1 optimizations:

1. Gather user feedback
2. Measure actual performance improvements
3. Proceed to Phase 2 (enhanced caching, hierarchical edges)
4. Continue iterating based on real-world usage

**Remember**: Start small, measure impact, iterate based on results.



