# Quick Reference - Refactored Modules

## Module Mapping Guide

### OCCGeometry Modules

| What you need | Include this | Module name |
|--------------|--------------|-------------|
| Basic geometry (shape, name) | `geometry/OCCGeometryCore.h` | OCCGeometryCore |
| Position, rotation, scale | `geometry/OCCGeometryTransform.h` | OCCGeometryTransform |
| Material colors (ambient, diffuse, specular) | `geometry/OCCGeometryMaterial.h` | OCCGeometryMaterial |
| Color, transparency, texture | `geometry/OCCGeometryAppearance.h` | OCCGeometryAppearance |
| Edges, vertices, wireframe | `geometry/OCCGeometryDisplay.h` | OCCGeometryDisplay |
| LOD, shadows, lighting | `geometry/OCCGeometryQuality.h` | OCCGeometryQuality |
| Mesh generation, Coin3D | `geometry/OCCGeometryMesh.h` | OCCGeometryMesh |
| Primitive shapes (Box, Sphere, etc.) | `geometry/OCCGeometryPrimitives.h` | OCCGeometryPrimitives |
| **Everything (recommended)** | `geometry/OCCGeometry.h` | **OCCGeometry** |

### OCCViewer Controllers

| What you need | Include this | Controller name |
|--------------|--------------|-----------------|
| Camera, fit, refresh | `viewer/ViewportController.h` | ViewportController |
| Wireframe, edges, normals | `viewer/RenderingController.h` | RenderingController |
| Mesh quality, tessellation | `viewer/MeshParameterController.h` | MeshParameterController |
| Level of detail | `viewer/LODController.h` | LODController |
| Clipping planes | `viewer/SliceController.h` | SliceController |
| Assembly explosion | `viewer/ExplodeController.h` | ExplodeController |
| Edge display | `edges/EdgeDisplayManager.h` | EdgeDisplayManager |
| **Everything (recommended)** | `viewer/OCCViewer.h` | **OCCViewer** |

### EdgeComponent Modules

| What you need | Include this | Module name |
|--------------|--------------|-------------|
| Extract edges from geometry | `edges/EdgeExtractor.h` | EdgeExtractor |
| Render edges with Coin3D | `edges/EdgeRenderer.h` | EdgeRenderer |
| **Everything (recommended)** | `edges/EdgeComponent.h` | **EdgeComponent** |

## Backward Compatibility

### Old Code (Still Works!)
```cpp
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "EdgeComponent.h"
```

### New Modular Code (Recommended)
```cpp
#include "geometry/OCCGeometry.h"
#include "viewer/OCCViewer.h"
#include "edges/EdgeComponent.h"
```

## Quick Function Finder

### OCCGeometry Methods

| Functionality | Method | Module |
|--------------|--------|--------|
| Get/set shape | `getShape()`, `setShape()` | Core |
| Get/set name | `getName()`, `setName()` | Core |
| Get/set position | `getPosition()`, `setPosition()` | Transform |
| Get/set rotation | `getRotation()`, `setRotation()` | Transform |
| Get/set scale | `getScale()`, `setScale()` | Transform |
| Get/set color | `getColor()`, `setColor()` | Appearance |
| Get/set transparency | `getTransparency()`, `setTransparency()` | Appearance |
| Get/set material | `setMaterialDiffuseColor()`, etc. | Material |
| Get/set texture | `setTextureEnabled()`, `setTextureImagePath()` | Appearance |
| Get/set display mode | `setDisplayMode()`, `setWireframeMode()` | Display |
| Get/set quality | `setRenderingQuality()`, `setTessellationLevel()` | Quality |
| Generate mesh | `buildCoinRepresentation()`, `regenerateMesh()` | Mesh |

### OCCViewer Methods

| Functionality | Method | Controller |
|--------------|--------|-----------|
| Fit all geometries | `fitAll()` | Viewport |
| Fit specific geometry | `fitGeometry(name)` | Viewport |
| Get camera position | `getCameraPosition()` | Viewport |
| Set wireframe mode | `setWireframeMode(bool)` | Rendering |
| Show/hide edges | `setShowEdges(bool)` | Rendering |
| Show/hide normals | `setShowNormals(bool)` | Rendering |
| Set mesh deflection | `setMeshDeflection(double)` | Mesh |
| Enable LOD | `setLODEnabled(bool)` | LOD |
| Enable slice | `setSliceEnabled(bool)` | Slice |
| Enable explode | `setExplodeEnabled(bool)` | Explode |
| Show original edges | `setShowOriginalEdges(bool)` | EdgeDisplay |
| Show feature edges | `setShowFeatureEdges(bool)` | EdgeDisplay |

### EdgeComponent Methods

| Functionality | Method | Module |
|--------------|--------|--------|
| Extract original edges | `extractOriginalEdges()` | Extractor |
| Extract feature edges | `extractFeatureEdges()` | Extractor |
| Extract mesh edges | `extractMeshEdges()` | Extractor |
| Extract silhouette | `generateSilhouetteEdgeNode()` | Extractor |
| Generate edge nodes | `generateOriginalEdgeNode()` | Renderer |
| Apply appearance | `applyAppearanceToEdgeNode()` | Renderer |
| Get edge node | `getEdgeNode(type)` | Renderer |

## Common Use Cases

### 1. Create and Display Geometry
```cpp
// Old way (still works)
#include "OCCGeometry.h"
#include "OCCViewer.h"

auto geom = std::make_shared<OCCGeometry>("myBox");
geom->setShape(myShape);
geom->setColor(Quantity_Color(1, 0, 0, Quantity_TOC_RGB));
viewer->addGeometry(geom);

// New way (same API)
#include "geometry/OCCGeometry.h"
#include "viewer/OCCViewer.h"

auto geom = std::make_shared<OCCGeometry>("myBox");
geom->setColor(Quantity_Color(1, 0, 0, Quantity_TOC_RGB));
viewer->addGeometry(geom);
```

### 2. Change Display Mode
```cpp
// Wireframe mode
viewer->setWireframeMode(true);

// Show edges
viewer->setShowEdges(true);

// Show normals
viewer->setShowNormals(true);
```

### 3. Extract and Display Edges
```cpp
// Get edge component from geometry
auto& edgeComp = geom->getEdgeComponent();

// Extract original edges
edgeComp->extractOriginalEdges(shape, 80.0, 0.01);

// Extract feature edges
edgeComp->extractFeatureEdges(shape, 15.0, 0.005, false, false);
```

### 4. Apply Material
```cpp
// Set material colors
geom->setMaterialDiffuseColor(Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB));
geom->setMaterialAmbientColor(Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB));
geom->setMaterialSpecularColor(Quantity_Color(1, 1, 1, Quantity_TOC_RGB));
geom->setMaterialShininess(50.0);
```

### 5. Control Quality
```cpp
// Set tessellation
geom->setTessellationLevel(3);

// Enable LOD
geom->setEnableLOD(true);
geom->setLODDistance(1000.0);

// Set shadow
geom->setShadowMode(RenderingConfig::ShadowMode::Soft);
geom->setShadowIntensity(0.5);
```

## Design Patterns Quick Reference

| Pattern | Where | Why |
|---------|-------|-----|
| **Composition** | OCCGeometry | Combines multiple modules into one class |
| **Controller** | OCCViewer | Delegates to specialized controllers |
| **Facade** | EdgeComponent | Simplifies complex edge subsystem |
| **Strategy** | Quality settings | Pluggable rendering algorithms |

## File Organization

```
Project Root
├── include/
│   ├── geometry/        # OCCGeometry modules (9 files)
│   ├── viewer/          # OCCViewer modules (9 files)
│   ├── edges/           # EdgeComponent modules (3 files)
│   ├── OCCGeometry.h    # Compatibility wrapper
│   ├── OCCViewer.h      # Compatibility wrapper
│   └── EdgeComponent.h  # Compatibility wrapper
│
├── src/
│   └── opencascade/     # Implementation files
│
└── docs/
    ├── REFACTORING_GUIDE.md        # Full guide (EN)
    ├── 重构说明.md                  # Full guide (CN)
    ├── REFACTORING_SUMMARY.md      # Summary
    ├── ARCHITECTURE_DIAGRAM.md     # Diagrams
    └── QUICK_REFERENCE.md          # This file
```

## Migration Checklist

- [ ] Replace monolithic includes with modular includes (optional)
- [ ] Review new module interfaces
- [ ] Update unit tests to test modules independently
- [ ] Consider using specific modules instead of main classes for better performance
- [ ] Document which modules your code depends on

## Tips

1. **Use compatibility wrappers** for existing code - no changes needed!
2. **Use modular includes** for new code - better organization
3. **Test modules independently** - easier debugging
4. **Refer to module docs** - each has clear responsibilities
5. **Check ARCHITECTURE_DIAGRAM.md** - visual reference

## Getting Help

| Document | Purpose |
|----------|---------|
| `REFACTORING_GUIDE.md` | Complete refactoring guide (EN) |
| `重构说明.md` | Complete refactoring guide (CN) |
| `REFACTORING_SUMMARY.md` | Executive summary |
| `ARCHITECTURE_DIAGRAM.md` | Visual architecture |
| `QUICK_REFERENCE.md` | This quick reference |
| `REFACTORING_COMPLETE.md` | Completion report |

---

**Happy Coding with the New Modular Architecture! 🚀**
