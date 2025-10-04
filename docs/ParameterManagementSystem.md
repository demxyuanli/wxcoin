# Unified Parameter Management System

## Overview

The Unified Parameter Management System provides a centralized, hierarchical approach to managing all geometry representation and drawing control parameters in the application. It establishes relationships between parameters and update/refresh interfaces, enabling intelligent batch updates and optimization.

## Architecture

The system consists of four main components:

### 1. Parameter Tree (`ParameterTree`)
- Hierarchical parameter storage with tree structure
- Support for different parameter types (bool, int, double, string, colors, enums)
- Path-based parameter access (e.g., "geometry/transform/position/x")
- Validation and callback support
- Serialization/deserialization capabilities

### 2. Parameter Update Manager (`ParameterUpdateManager`)
- Maps parameter changes to update types (Geometry, Rendering, Display, etc.)
- Manages update priorities and batching
- Optimizes update frequency and prevents duplicate calls
- Supports different update strategies

### 3. Parameter Synchronizer (`ParameterSynchronizer`)
- Synchronizes parameters between the tree and existing systems
- Supports bidirectional synchronization
- Manages geometry and rendering config integration
- Handles batch synchronization operations

### 4. Unified Parameter Manager (`UnifiedParameterManager`)
- Main interface for the entire system
- Coordinates all subsystems
- Provides high-level API for parameter operations
- Manages system integration and initialization

## Key Features

### Hierarchical Parameter Organization
```
root/
├── geometry/
│   ├── transform/
│   │   ├── position/ (x, y, z)
│   │   ├── rotation/ (axis, angle)
│   │   └── scale
│   ├── display/ (visible, selected, wireframe_mode)
│   ├── color/ (main, edge, vertex)
│   └── quality/ (tessellation_level, deflection)
├── rendering/
│   ├── mode/ (display_mode, shading_mode, quality)
│   └── features/ (show_edges, smooth_normals, lod)
├── material/
│   ├── color/ (ambient, diffuse, specular, emissive)
│   └── properties/ (shininess, transparency, metallic)
├── lighting/
│   ├── model/ (lighting_model, roughness, metallic)
│   ├── ambient/ (color, intensity)
│   ├── diffuse/ (color, intensity)
│   └── specular/ (color, intensity)
├── texture/
│   ├── enabled, image_path
│   ├── color/ (main)
│   ├── mode/ (texture_mode)
│   └── coordinates/ (repeat_u, repeat_v, offset_u, offset_v)
├── shadow/
│   ├── mode/ (shadow_mode, enabled)
│   ├── intensity/ (shadow_intensity, shadow_softness)
│   └── quality/ (shadow_map_size, shadow_bias)
└── quality/
    ├── level/ (rendering_quality, tessellation_level)
    ├── antialiasing/ (samples, enabled)
    ├── lod/ (enabled, distance, levels)
    └── subdivision/ (enabled, levels)
```

### Intelligent Update Management
- **Update Type Mapping**: Parameters are mapped to specific update types (Geometry, Rendering, Display, etc.)
- **Priority System**: Updates are prioritized (Low, Normal, High, Critical)
- **Batch Optimization**: Multiple parameter changes are batched and optimized
- **Frequency Limiting**: Prevents excessive update calls
- **Duplicate Elimination**: Removes redundant update tasks

### System Integration
- **Geometry Objects**: Automatic synchronization with OCCGeometry instances
- **Rendering Config**: Integration with RenderingConfig system
- **Update Interfaces**: Pluggable update interface system
- **Callback System**: Event-driven parameter change notifications

## Usage Examples

### Basic Parameter Operations
```cpp
auto& manager = UnifiedParameterManager::getInstance();
manager.initialize();

// Set parameters
manager.setParameter("geometry/transform/position/x", 10.0);
manager.setParameter("material/color/diffuse", Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB));
manager.setParameter("rendering/mode/display_mode", static_cast<int>(RenderingConfig::DisplayMode::Solid));

// Get parameters
double x = manager.getParameter("geometry/transform/position/x").getValueAs<double>();
auto color = manager.getParameter("material/color/diffuse").getValueAs<Quantity_Color>();
```

### Geometry Integration
```cpp
// Create and register geometry
auto geometry = std::make_shared<OCCBox>("TestBox", 10.0, 10.0, 10.0);
manager.registerGeometry(geometry);

// Parameters automatically sync to geometry
manager.setParameter("geometry/color/main", Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
manager.setParameter("geometry/transparency", 0.3);

// Unregister when done
manager.unregisterGeometry(geometry);
```

### Batch Operations
```cpp
// Begin batch operation
manager.beginBatchOperation();

// Set multiple parameters
manager.setParameter("geometry/transform/position/x", 100.0);
manager.setParameter("geometry/transform/position/y", 200.0);
manager.setParameter("geometry/transform/position/z", 300.0);
manager.setParameter("material/color/diffuse", Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB));

// End batch operation (triggers optimized updates)
manager.endBatchOperation();
```

### Performance Optimization
```cpp
// Enable optimization features
manager.enableOptimization(true);
manager.setUpdateFrequencyLimit(30); // Limit to 30 updates per second
manager.enableDebugMode(true);
```

## Integration with Existing Systems

### RenderingConfig Integration
The system automatically synchronizes with the existing RenderingConfig singleton:
- Material settings (colors, shininess, transparency)
- Lighting settings (ambient, diffuse, specular)
- Rendering modes (display mode, shading mode, quality)
- Shadow settings (mode, intensity, quality)

### OCCGeometry Integration
Geometry objects are automatically synchronized with:
- Transform parameters (position, rotation, scale)
- Display properties (visible, selected, wireframe mode)
- Material properties (colors, transparency, shininess)
- Quality settings (tessellation level, deflection)

## Performance Considerations

### Update Optimization
- **Batching**: Multiple parameter changes are batched together
- **Frequency Limiting**: Updates are throttled to prevent excessive calls
- **Duplicate Elimination**: Redundant updates are removed
- **Priority-based Execution**: Critical updates are processed first

### Memory Management
- **Shared Pointers**: Automatic memory management for update interfaces
- **Lazy Initialization**: Parameters are created only when needed
- **Efficient Storage**: Tree structure minimizes memory overhead

### Thread Safety
- **Mutex Protection**: All operations are thread-safe
- **Atomic Operations**: Batch state management uses atomic operations
- **Lock-free Design**: Where possible, lock-free data structures are used

## Building and Integration

### CMake Integration
```cmake
# Add parameter management library
add_subdirectory(src/param)

# Link to your target
target_link_libraries(your_target param_management)
```

### Dependencies
- OpenCASCADE (for geometry types)
- jsoncpp (for serialization)
- C++17 standard library
- Threading support

## Future Enhancements

### Planned Features
- **Parameter Presets**: Save and load parameter configurations
- **Animation Support**: Animated parameter transitions
- **Undo/Redo**: Parameter change history
- **Validation Rules**: Complex parameter validation
- **Plugin System**: Extensible parameter types and update strategies

### Performance Improvements
- **Lazy Updates**: Defer non-critical updates
- **Dirty Tracking**: Only update changed components
- **Parallel Processing**: Multi-threaded update execution
- **Memory Pooling**: Reuse update task objects

## Troubleshooting

### Common Issues
1. **Parameter Not Found**: Ensure parameter path is correct and parameter is registered
2. **Update Not Triggered**: Check if update interface is properly registered
3. **Performance Issues**: Enable optimization and adjust frequency limits
4. **Synchronization Problems**: Verify bidirectional sync is properly configured

### Debug Mode
Enable debug mode to see parameter changes and update operations:
```cpp
manager.enableDebugMode(true);
```

This will output detailed information about parameter changes and update operations to the console.