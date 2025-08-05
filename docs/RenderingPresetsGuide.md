# Rendering Presets Guide

## Overview

The rendering system now includes comprehensive preset configurations for both anti-aliasing and rendering modes. These presets are designed to provide optimal settings for different use cases and performance requirements.

## Anti-Aliasing Presets

### Basic Presets
- **None**: No anti-aliasing applied
- **MSAA 2x/4x/8x/16x**: Multi-Sample Anti-Aliasing with different sample counts
- **FXAA Low/Medium/High/Ultra**: Fast Approximate Anti-Aliasing with quality levels
- **SSAA 2x/4x**: Super-Sample Anti-Aliasing for high quality
- **TAA Low/Medium/High**: Temporal Anti-Aliasing for temporal stability

### Hybrid Presets
- **MSAA 4x + FXAA**: Combines MSAA with FXAA for enhanced quality
- **MSAA 4x + TAA**: Combines MSAA with TAA for temporal stability

### Performance Optimized
- **Performance Low**: Fast FXAA with minimal impact
- **Performance Medium**: Balanced MSAA for good performance

### Quality Optimized
- **Quality High**: High quality MSAA with advanced features
- **Quality Ultra**: Maximum quality with SSAA

### Application Specific
- **CAD Standard**: Optimized for CAD applications
- **CAD High Quality**: Professional CAD quality
- **Gaming Fast**: Fast rendering for real-time applications
- **Gaming Balanced**: Balanced for gaming performance
- **Mobile Low/Medium**: Optimized for mobile devices

## Rendering Mode Presets

### Basic Presets
- **Performance**: Fast rendering with minimal quality
- **Balanced**: Good balance of quality and performance
- **Quality**: High quality rendering
- **Ultra**: Maximum quality rendering
- **Wireframe**: Wireframe rendering mode

### CAD/Engineering Presets
- **CAD Standard**: Standard CAD rendering with high precision
- **CAD High Quality**: Professional CAD quality
- **CAD Wireframe**: Wireframe mode for CAD work

### Gaming Presets
- **Gaming Fast**: Fast rendering for real-time applications
- **Gaming Balanced**: Balanced for gaming performance
- **Gaming Quality**: High quality for gaming

### Mobile Presets
- **Mobile Low**: Low power consumption for mobile devices
- **Mobile Medium**: Medium quality for mobile devices

### Presentation Presets
- **Presentation Standard**: High quality for presentations
- **Presentation High**: Ultra high quality for professional presentations

### Debug Presets
- **Debug Wireframe**: Wireframe mode for debugging
- **Debug Points**: Points mode for debugging

### Legacy Presets
- **Legacy Solid**: Traditional solid rendering
- **Legacy Hidden Line**: Traditional hidden line rendering

## Performance Impact

The system provides real-time performance impact assessment:

- **Low Impact** (Green): Minimal performance impact
- **Medium Impact** (Orange): Moderate performance impact
- **High Impact** (Red): Significant performance impact

## Estimated FPS

The system estimates frame rates based on:
- Anti-aliasing method and quality
- Rendering mode and features
- Hardware capabilities
- Scene complexity

## Configuration Files

### render_settings.ini
Contains current rendering settings and user preferences.

### rendering_presets.ini
Contains user preferred presets and UI preferences:
- Default presets for different scenarios
- Performance thresholds
- Quality descriptions
- Feature descriptions
- User interface preferences

## Usage

1. **Select Preset**: Choose from the dropdown menus in the Anti-aliasing and Rendering Mode tabs
2. **Auto Apply**: Enable auto-apply to automatically apply changes
3. **Manual Apply**: Use the Apply button to manually apply settings
4. **Save Settings**: Use the Save button to persist your preferences
5. **Reset**: Use the Reset button to restore default settings

## Customization

Users can customize presets by:
1. Modifying the rendering_presets.ini file
2. Creating custom preset configurations
3. Adjusting performance thresholds
4. Customizing quality descriptions

## Best Practices

### For CAD Applications
- Use CAD Standard or CAD High Quality presets
- Enable backface culling
- Use MSAA for anti-aliasing

### For Gaming
- Use Gaming Fast for maximum performance
- Use Gaming Balanced for good balance
- Use Gaming Quality for high quality

### For Mobile Devices
- Use Mobile Low for battery optimization
- Use Mobile Medium for better quality
- Enable adaptive quality features

### For Presentations
- Use Presentation Standard or Presentation High
- Enable smooth shading
- Use high quality anti-aliasing

## Technical Details

### Anti-Aliasing Methods
- **MSAA**: Hardware-accelerated multi-sampling
- **FXAA**: Fast approximate anti-aliasing
- **SSAA**: Super-sampling anti-aliasing
- **TAA**: Temporal anti-aliasing

### Rendering Features
- **Smooth Shading**: Interpolated vertex colors
- **Phong Shading**: Per-pixel lighting calculation
- **Backface Culling**: Remove back-facing polygons
- **Frustum Culling**: Remove off-screen objects
- **Occlusion Culling**: Remove hidden objects
- **Adaptive Quality**: Dynamic quality adjustment

### Performance Optimization
- **Fast Mode**: Reduced quality for speed
- **Adaptive Quality**: Dynamic quality adjustment
- **Culling**: Remove unnecessary geometry
- **LOD**: Level of detail adjustment 