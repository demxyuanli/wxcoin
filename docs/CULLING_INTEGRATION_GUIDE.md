# Culling System Integration Guide

## Overview
The culling system has been integrated into the main rendering loop to optimize performance by excluding objects outside the view frustum or fully occluded by other objects.

## Features
- **View Frustum Culling**: Automatically excludes objects outside the camera's view frustum
- **Occlusion Culling**: Skips rendering of objects completely hidden by other objects
- **Performance Monitoring**: Provides statistics on culling effectiveness
- **Dynamic Control**: Enable/disable culling features at runtime

## Integration Points

### 1. SceneManager Integration
The culling system is automatically initialized in `SceneManager::initScene()`:
```cpp
// Culling is enabled by default
m_renderingToolkit->setFrustumCullingEnabled(true);
m_renderingToolkit->setOcclusionCullingEnabled(true);
```

### 2. Rendering Loop Integration
Culling is updated before each render frame in `SceneManager::render()`:
```cpp
// Update culling system before rendering
if (m_cullingEnabled && m_renderingToolkit) {
    updateCulling();
}
```

### 3. Geometry Creation Integration
Large objects are automatically added as occluders in `GeometryFactory`:
```cpp
// Add to culling system as occluder if it's a large object
if (type == "Box" || type == "Cylinder" || type == "Cone") {
    addGeometryToCullingSystem(geometry);
}
```

## Usage Examples

### Basic Usage
```cpp
// Get SceneManager instance
SceneManager* sceneManager = canvas->getSceneManager();

// Check if a shape should be rendered
if (sceneManager->shouldRenderShape(myShape)) {
    // Render the shape
    renderShape(myShape);
}

// Add an object as an occluder
sceneManager->addOccluder(largeObjectShape);

// Remove an object from occluders
sceneManager->removeOccluder(largeObjectShape);
```

### Performance Monitoring
```cpp
// Get culling statistics
CullingStats stats = sceneManager->getCullingStats();
std::cout << "Frustum culled: " << stats.frustumCulled << std::endl;
std::cout << "Occlusion culled: " << stats.occlusionCulled << std::endl;
std::cout << "Total objects: " << stats.totalObjects << std::endl;
```

### Runtime Control
```cpp
// Enable/disable frustum culling
sceneManager->setFrustumCullingEnabled(true);

// Enable/disable occlusion culling
sceneManager->setOcclusionCullingEnabled(false);
```

## Configuration

### Automatic Occluder Selection
The system automatically adds large objects (Box, Cylinder, Cone) as occluders. To customize:

1. Modify `GeometryFactory::addGeometryToCullingSystem()`
2. Add your own logic for occluder selection
3. Call `sceneManager->addOccluder()` manually

### Culling Parameters
Adjust culling sensitivity and performance:
```cpp
// In RenderingToolkitAPI
setFrustumCullingEnabled(bool enabled);
setOcclusionCullingEnabled(bool enabled);
```

## Performance Benefits

### Expected Improvements
- **View Frustum Culling**: 20-50% reduction in GPU load for scenes with many objects
- **Occlusion Culling**: 10-30% additional reduction for complex scenes with overlapping objects
- **Overall**: 30-80% performance improvement depending on scene complexity

### Monitoring
The system logs culling statistics every 60 frames:
```
Culling stats - Frustum culled: 15, Occlusion culled: 8, Total objects: 50
```

## Troubleshooting

### Common Issues
1. **Objects not rendering**: Check if culling is too aggressive
2. **Performance not improving**: Verify culling is enabled and objects are being culled
3. **Incorrect culling**: Ensure camera matrix extraction is working correctly

### Debug Mode
Enable debug logging to see culling decisions:
```cpp
// Check culling stats
auto stats = sceneManager->getCullingStats();
if (stats.frustumCulled > 0 || stats.occlusionCulled > 0) {
    LOG_INF_S("Culling is working: " + std::to_string(stats.frustumCulled) + 
              " frustum, " + std::to_string(stats.occlusionCulled) + " occlusion");
}
```

## Advanced Usage

### Custom Culling Logic
Implement custom culling by extending the culling system:
```cpp
// Override shouldRenderShape for custom logic
bool MySceneManager::shouldRenderShape(const TopoDS_Shape& shape) const {
    // Add your custom culling logic here
    if (myCustomCullingCondition(shape)) {
        return false; // Don't render
    }
    
    // Call parent implementation
    return SceneManager::shouldRenderShape(shape);
}
```

### Dynamic Occluder Management
```cpp
// Add temporary occluders
sceneManager->addOccluder(temporaryObject);

// Remove when no longer needed
sceneManager->removeOccluder(temporaryObject);
```

## Integration Checklist

- [x] Culling system compiled successfully
- [x] SceneManager integration complete
- [x] Rendering loop integration complete
- [x] Geometry creation integration complete
- [x] Performance monitoring enabled
- [x] Runtime controls available
- [ ] Test with various scene complexities
- [ ] Verify performance improvements
- [ ] Document any custom requirements 