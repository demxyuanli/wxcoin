# Lighting Settings Guide

## Overview

The lighting settings system provides comprehensive control over scene illumination, allowing users to configure multiple light sources, environment settings, and lighting presets. This system replaces the previous hardcoded lighting setup with a flexible, configurable solution.

## Features

### 1. Multiple Light Sources
- **Directional Lights**: Primary light sources with direction, color, and intensity
- **Point Lights**: Omni-directional lights with position and attenuation
- **Spot Lights**: Focused lights with angle and falloff control
- **Environment Lighting**: Ambient lighting for overall scene brightness

### 2. Light Properties
Each light source supports the following properties:
- **Name**: User-friendly identifier
- **Type**: Light type (directional, point, spot, ambient)
- **Enabled/Disabled**: Toggle light on/off
- **Position**: 3D coordinates for point and spot lights
- **Direction**: 3D direction vector for directional lights
- **Color**: RGB color values
- **Intensity**: Light strength (0.0 to 2.0)
- **Spot Angle**: Cone angle for spot lights (1° to 90°)
- **Spot Exponent**: Falloff rate for spot lights
- **Attenuation**: Distance-based falloff (constant, linear, quadratic)

### 3. Environment Settings
- **Ambient Color**: Overall scene color tint
- **Ambient Intensity**: Background lighting strength

### 4. Lighting Presets
Pre-configured lighting setups for common scenarios:
- **Studio**: Professional studio lighting with key, fill, and rim lights
- **Outdoor**: Natural outdoor lighting with sun and sky simulation
- **Dramatic**: High-contrast lighting for dramatic effects

## Usage

### Accessing Lighting Settings
1. Click the "Lighting Settings" button in the Tools panel
2. The lighting settings dialog will open with three tabs:
   - **Environment**: Ambient lighting configuration
   - **Lights**: Individual light source management
   - **Presets**: Quick lighting setup selection

### Environment Configuration
1. **Ambient Color**: Click the color button to choose the overall scene tint
2. **Ambient Intensity**: Use the slider to adjust background lighting strength (0% to 100%)

### Light Management
1. **Light List**: Select a light from the list to edit its properties
2. **Add Light**: Click "Add Light" to create a new light source
3. **Remove Light**: Select a light and click "Remove Light" to delete it
4. **Light Properties**:
   - **Name**: Enter a descriptive name
   - **Type**: Choose from directional, point, or spot
   - **Enabled**: Check to enable the light
   - **Position/Direction**: Set 3D coordinates or direction vector
   - **Color**: Click to choose light color
   - **Intensity**: Adjust light strength
   - **Spot Settings**: Configure angle and exponent for spot lights
   - **Attenuation**: Set distance falloff for point/spot lights

### Using Presets
1. Select a preset from the dropdown menu
2. Click "Apply Preset" to load the configuration
3. The settings will be applied to the temporary configuration
4. Click "Apply" to test in the scene
5. Click "OK" to save and apply permanently

### Workflow
1. **Test Settings**: Use "Apply" to test changes in the scene without saving
2. **Save Configuration**: Use "OK" to save settings to file and apply permanently
3. **Reset**: Use "Reset" to restore default lighting setup
4. **Cancel**: Use "Cancel" to discard changes

## Configuration File

Lighting settings are saved to `lighting_settings.ini` in the application root directory. The file structure:

```ini
[Environment]
AmbientColor=0.4,0.4,0.4
AmbientIntensity=1.0

[Light0]
Name=Main Light
Type=directional
Enabled=true
PositionX=0.0
PositionY=0.0
PositionZ=0.0
DirectionX=0.5
DirectionY=0.5
DirectionZ=-1.0
Color=1.0,1.0,1.0
Intensity=1.0
SpotAngle=30.0
SpotExponent=1.0
ConstantAttenuation=1.0
LinearAttenuation=0.0
QuadraticAttenuation=0.0

[Light1]
Name=Fill Light
Type=directional
Enabled=true
...
```

## Technical Details

### Integration with Scene Manager
- Lighting changes are applied immediately to the scene
- Coin3D lighting nodes are updated dynamically
- Scene refresh is triggered automatically
- Multiple refresh methods ensure reliable updates

### Default Configuration
The system initializes with a three-light setup:
1. **Main Light**: Primary directional light (white, intensity 1.0)
2. **Fill Light**: Secondary directional light (slightly blue, intensity 0.6)
3. **Back Light**: Rim lighting (slightly yellow, intensity 0.5)

### Performance Considerations
- Light count affects rendering performance
- Disabled lights are not processed
- Attenuation calculations impact performance for point/spot lights
- Environment settings have minimal performance impact

## Troubleshooting

### Common Issues
1. **Lights not visible**: Check if lights are enabled and positioned correctly
2. **Scene too dark**: Increase ambient intensity or add more lights
3. **Scene too bright**: Reduce light intensities or ambient intensity
4. **Settings not saving**: Check file permissions in application directory

### Debug Information
The system provides detailed logging for troubleshooting:
- Light creation and modification events
- Configuration file operations
- Scene update notifications
- Error conditions and resolutions

## Future Enhancements

Planned features for future versions:
- **Area Lights**: Soft lighting from surface emitters
- **IES Profiles**: Real-world light distribution patterns
- **Light Animation**: Animated lighting for presentations
- **Light Baking**: Pre-computed lighting for performance
- **HDR Environment Maps**: Image-based lighting support 