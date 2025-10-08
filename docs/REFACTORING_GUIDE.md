# Code Refactoring Guide - Modular Architecture

## Overview

The three main packages (`OCCGeometry`, `OCCViewer`, `EdgeComponent`) have been refactored into smaller, focused modules for better maintainability and management. This document describes the new architecture.

## 1. OCCGeometry Refactoring

### Previous Structure
- **OCCGeometry.h/cpp** (~2050 lines): Single monolithic class with all functionality

### New Modular Structure

#### 1.1 OCCGeometryCore (`geometry/OCCGeometryCore.h`)
- **Purpose**: Core geometry data and basic operations
- **Responsibilities**:
  - Shape management (TopoDS_Shape)
  - Name and file information
  - Basic validity checks

#### 1.2 OCCGeometryTransform (`geometry/OCCGeometryTransform.h`)
- **Purpose**: Transformation properties
- **Responsibilities**:
  - Position (gp_Pnt)
  - Rotation (axis + angle)
  - Scale
  - Coin3D transform node management

#### 1.3 OCCGeometryMaterial (`geometry/OCCGeometryMaterial.h`)
- **Purpose**: Material properties
- **Responsibilities**:
  - Ambient, diffuse, specular, emissive colors
  - Shininess
  - Material presets (bright material, lighting updates)
  - Coin3D material node management

#### 1.4 OCCGeometryAppearance (`geometry/OCCGeometryAppearance.h`)
- **Purpose**: Visual appearance settings
- **Responsibilities**:
  - Color and transparency
  - Visibility and selection state
  - Texture properties (color, intensity, image path, mode)
  - Blend settings (mode, depth test/write, culling, alpha threshold)

#### 1.5 OCCGeometryDisplay (`geometry/OCCGeometryDisplay.h`)
- **Purpose**: Display mode settings
- **Responsibilities**:
  - Display mode (shaded, wireframe, points)
  - Edge display (show, width, color)
  - Vertex display (show, size, color)
  - Wireframe settings
  - Face visibility control
  - Normal smoothing

#### 1.6 OCCGeometryQuality (`geometry/OCCGeometryQuality.h`)
- **Purpose**: Rendering quality settings
- **Responsibilities**:
  - Tessellation level and quality
  - Anti-aliasing samples
  - LOD (Level of Detail) settings
  - Shadow settings (mode, intensity, softness, map size, bias)
  - Lighting model (roughness, metallic, fresnel, subsurface scattering)
  - Advanced parameters (smoothing, subdivision, adaptive meshing)

#### 1.7 OCCGeometryMesh (`geometry/OCCGeometryMesh.h`)
- **Purpose**: Mesh generation and management
- **Responsibilities**:
  - Coin3D node management
  - Mesh generation and regeneration
  - Edge component integration
  - Face index mapping (for picking)
  - Assembly level tracking
  - Memory optimization

#### 1.8 OCCGeometryPrimitives (`geometry/OCCGeometryPrimitives.h`)
- **Purpose**: Primitive shape definitions
- **Classes**:
  - OCCBox
  - OCCCylinder
  - OCCSphere
  - OCCCone
  - OCCTorus
  - OCCTruncatedCylinder

#### 1.9 OCCGeometry (Main Class) (`geometry/OCCGeometry.h`)
- **Purpose**: Main geometry class using composition pattern
- **Implementation**: Delegates to all specialized modules
- **Pattern**: Composition over inheritance

### Migration Path for OCCGeometry
```cpp
// Old code (still works via compatibility wrapper)
#include "OCCGeometry.h"
OCCGeometry* geom = new OCCGeometry("myShape");

// New modular approach (if needed)
#include "geometry/OCCGeometry.h"
OCCGeometry* geom = new OCCGeometry("myShape");
// Access still works the same way
geom->setColor(color);
geom->setPosition(pos);
```

## 2. OCCViewer Refactoring

### Previous Structure
- **OCCViewer.h/cpp** (~1518 lines): Single class with all viewer functionality

### New Modular Structure

#### 2.1 ViewportController (`viewer/ViewportController.h`)
- **Purpose**: Viewport and view manipulation
- **Responsibilities**:
  - Camera positioning
  - Fit operations (fitAll, fitGeometry)
  - View refresh
  - Preserve view settings

#### 2.2 RenderingController (`viewer/RenderingController.h`)
- **Purpose**: Rendering and display mode management
- **Responsibilities**:
  - Wireframe mode
  - Edge display
  - Anti-aliasing
  - Normal visualization
  - Normal consistency and debug modes

#### 2.3 Existing Controllers (Already Modular)
- **MeshParameterController**: Mesh quality control
- **LODController**: Level of detail
- **SliceController**: Clipping planes
- **ExplodeController**: Assembly explosion
- **EdgeDisplayManager**: Edge display management
- **OutlineDisplayManager**: Outline effects

#### 2.4 OCCViewer (Main Class) (`viewer/OCCViewer.h`)
- **Purpose**: Main viewer class
- **Implementation**: Delegates to specialized controllers
- **Pattern**: Composition with controller pattern

### Migration Path for OCCViewer
```cpp
// Old code (still works via compatibility wrapper)
#include "OCCViewer.h"
OCCViewer* viewer = new OCCViewer(sceneManager);

// New modular approach (if needed)
#include "viewer/OCCViewer.h"
OCCViewer* viewer = new OCCViewer(sceneManager);
// All existing APIs work the same
viewer->setWireframeMode(true);
viewer->fitAll();
```

## 3. EdgeComponent Refactoring

### Previous Structure
- **EdgeComponent.h/cpp** (~1694 lines): Combined extraction and rendering

### New Modular Structure

#### 3.1 EdgeExtractor (`edges/EdgeExtractor.h`)
- **Purpose**: Edge extraction logic
- **Responsibilities**:
  - Extract original edges from CAD geometry
  - Extract feature edges (angle-based)
  - Extract mesh edges
  - Extract silhouette edges
  - Find edge intersections

#### 3.2 EdgeRenderer (`edges/EdgeRenderer.h`)
- **Purpose**: Edge visualization
- **Responsibilities**:
  - Generate Coin3D nodes for all edge types
  - Apply appearance (color, width, style)
  - Manage edge node lifecycle
  - Update edge display in scene

#### 3.3 EdgeComponent (Main Class) (`edges/EdgeComponent.h`)
- **Purpose**: High-level edge interface
- **Implementation**: Combines EdgeExtractor and EdgeRenderer
- **Pattern**: Facade pattern

### Migration Path for EdgeComponent
```cpp
// Old code (still works via compatibility wrapper)
#include "EdgeComponent.h"
EdgeComponent* edges = new EdgeComponent();

// New modular approach (if needed)
#include "edges/EdgeComponent.h"
EdgeComponent* edges = new EdgeComponent();
// All existing APIs work the same
edges->extractOriginalEdges(shape, density, minLength);
```

## 4. Benefits of Refactoring

### 4.1 Maintainability
- **Smaller modules**: Easier to understand and modify
- **Clear responsibilities**: Each module has a focused purpose
- **Reduced coupling**: Modules are independent

### 4.2 Testability
- **Unit testing**: Each module can be tested independently
- **Mock objects**: Easier to create mocks for interfaces
- **Isolated changes**: Changes to one module don't affect others

### 4.3 Reusability
- **Composition**: Modules can be reused in different contexts
- **Flexibility**: Easy to add new modules or replace existing ones
- **Extension**: New functionality can be added without modifying existing code

### 4.4 Code Organization
- **Logical grouping**: Related functionality is grouped together
- **Clear structure**: Directory structure reflects functionality
- **Documentation**: Easier to document smaller modules

## 5. Directory Structure

```
include/
├── geometry/                    # Geometry modules
│   ├── OCCGeometry.h           # Main geometry class
│   ├── OCCGeometryCore.h       # Core geometry data
│   ├── OCCGeometryTransform.h  # Transform properties
│   ├── OCCGeometryMaterial.h   # Material properties
│   ├── OCCGeometryAppearance.h # Appearance settings
│   ├── OCCGeometryDisplay.h    # Display modes
│   ├── OCCGeometryQuality.h    # Quality settings
│   ├── OCCGeometryMesh.h       # Mesh management
│   └── OCCGeometryPrimitives.h # Primitive shapes
│
├── viewer/                      # Viewer modules
│   ├── OCCViewer.h             # Main viewer class
│   ├── ViewportController.h    # Viewport control
│   ├── RenderingController.h   # Rendering control
│   ├── MeshParameterController.h
│   ├── LODController.h
│   ├── SliceController.h
│   ├── ExplodeController.h
│   └── ...
│
├── edges/                       # Edge modules
│   ├── EdgeComponent.h         # Main edge class
│   ├── EdgeExtractor.h         # Edge extraction
│   ├── EdgeRenderer.h          # Edge rendering
│   └── EdgeDisplayManager.h
│
├── OCCGeometry.h               # Compatibility wrapper
├── OCCViewer.h                 # Compatibility wrapper
└── EdgeComponent.h             # Compatibility wrapper
```

## 6. Backward Compatibility

All existing code continues to work without changes. The compatibility wrappers in the root `include/` directory forward to the new modular headers:

- `include/OCCGeometry.h` → `include/geometry/OCCGeometry.h`
- `include/OCCViewer.h` → `include/viewer/OCCViewer.h`
- `include/EdgeComponent.h` → `include/edges/EdgeComponent.h`

## 7. Implementation Status

### Completed
✅ Header files for all geometry modules  
✅ Header files for viewer controllers  
✅ Header files for edge modules  
✅ Compatibility wrappers  
✅ Documentation  

### Pending
⏳ Implementation (.cpp) files for new modules  
⏳ CMakeLists.txt updates  
⏳ Migration of existing code to new modules  
⏳ Unit tests for new modules  

## 8. Next Steps

1. **Create implementation files**: Implement the new module .cpp files
2. **Update CMakeLists.txt**: Add new source files to build system
3. **Migrate existing code**: Move implementation from monolithic classes to modules
4. **Add tests**: Create unit tests for each module
5. **Update documentation**: Add API documentation for each module
6. **Performance testing**: Ensure refactoring doesn't impact performance

## 9. Guidelines for Developers

### Adding New Functionality
1. Identify the appropriate module
2. Add methods to the module interface
3. Implement in the module .cpp file
4. Update the main class to expose the functionality
5. Add tests

### Modifying Existing Functionality
1. Find the relevant module
2. Modify the module implementation
3. Ensure backward compatibility
4. Update tests

### Creating New Modules
1. Follow the existing module pattern
2. Create header in appropriate subdirectory
3. Implement in corresponding src subdirectory
4. Add to CMakeLists.txt
5. Document the module purpose and responsibilities
