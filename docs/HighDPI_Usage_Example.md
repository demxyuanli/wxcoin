# High-DPI Optimization Usage Examples

This document demonstrates how to use the new high-DPI optimization features.

## Overview of High-DPI Features

The application now includes comprehensive high-DPI support with three main components:

1. **DPIManager** - Centralized DPI scaling management
2. **DPIAwareRendering** - Rendering utilities for DPI-aware graphics  
3. **Automatic Canvas Adaptation** - Dynamic UI scaling

## Key Features

### 1. Font DPI Adaptation

Fonts automatically scale with system DPI settings:

```cpp
// Example: Creating DPI-aware fonts
auto& dpiManager = DPIManager::getInstance();

// Scale an existing font
wxFont baseFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
wxFont scaledFont = dpiManager.getScaledFont(baseFont);

// Create a new DPI-aware font
wxFont dpiFont = dpiManager.getScaledFont(16, "Arial", true, false);
```

### 2. Line Width Dynamic Scaling

Line widths automatically adjust for high-DPI displays:

```cpp
// Example: DPI-aware line width settings
#include "DPIAwareRendering.h"

// For OpenGL direct rendering
DPIAwareRendering::setDPIAwareLineWidth(1.0f);
DPIAwareRendering::setDPIAwarePointSize(2.0f);

// For Coin3D draw styles
SoDrawStyle* style = DPIAwareRendering::createDPIAwareCoordinateLineStyle(1.5f);

// For geometry outlines
SoDrawStyle* geometryStyle = DPIAwareRendering::createDPIAwareGeometryStyle(1.0f, false);
```

### 3. High-Resolution Texture Support

Textures automatically generate at optimal resolution for the current DPI:

```cpp
// Example: DPI-aware texture generation
auto& dpiManager = DPIManager::getInstance();

auto textureInfo = dpiManager.getOrCreateScaledTexture(
    "texture_key",           // Unique identifier
    128,                     // Base texture size
    [](unsigned char* data, int width, int height) -> bool {
        return generateCustomTexture(data, width, height);
    }
);
```

## Benefits

- **Consistent Visual Quality**: All elements scale proportionally
- **Performance Optimized**: Intelligent caching and power-of-2 texture sizes
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **Backward Compatible**: Functions gracefully on low-DPI displays
- **Developer Friendly**: Simple API with automatic management 