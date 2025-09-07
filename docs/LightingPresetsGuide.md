# Lighting Presets Guide

## Overview
The lighting system includes 6 pre-configured lighting presets that can be applied instantly to your 3D scene. Each preset provides a different lighting atmosphere and is designed to showcase different aspects of your models.

## How to Use Presets

1. Open the Lighting Settings dialog from the menu
2. Click on the "Presets" tab
3. Click any preset button to immediately apply that lighting setup
4. The scene will update instantly with the new lighting
5. You can switch between presets at any time

## Available Presets

### 1. Studio Lighting
- **Purpose**: Professional product photography style
- **Characteristics**: 
  - Key light from front-right
  - Fill light from left to reduce shadows
  - Rim light from back to separate objects from background
  - Low ambient light for controlled shadows
- **Best for**: Product visualization, technical presentations

### 2. Outdoor Lighting
- **Purpose**: Natural daylight simulation
- **Characteristics**:
  - Warm sun light from above-right
  - Cool sky light from above
  - Medium ambient light for realistic outdoor feel
- **Best for**: Architectural models, outdoor scenes

### 3. Dramatic Lighting
- **Purpose**: High contrast, artistic lighting
- **Characteristics**:
  - Strong main light from side
  - Very low ambient light
  - Deep shadows and high contrast
- **Best for**: Artistic renders, dramatic presentations

### 4. Warm Lighting
- **Purpose**: Cozy, inviting atmosphere
- **Characteristics**:
  - Orange/yellow tinted lights
  - Warm ambient light
  - Soft, comfortable feel
- **Best for**: Interior scenes, warm environments

### 5. Cool Lighting
- **Purpose**: Modern, technical appearance
- **Characteristics**:
  - Blue-tinted lights
  - Cool ambient light
  - Clean, modern feel
- **Best for**: Technical models, modern designs

### 6. Minimal Lighting
- **Purpose**: Simple, clean lighting
- **Characteristics**:
  - Single main light
  - Subtle fill light
  - High ambient light for soft shadows
- **Best for**: Simple presentations, basic visualization

## Technical Details

Each preset configures:
- **Environment lighting**: Ambient color and intensity
- **Directional lights**: Position, direction, color, and intensity
- **Light count**: Number of lights (1-3 depending on preset)

## Customization

After applying a preset, you can:
1. Switch to the "Environment Lighting" tab to adjust ambient settings
2. Switch to the "Light Management" tab to modify individual lights
3. Use the "Apply" button to save your customizations
4. Use the "Reset" button to return to the original preset

## Tips for Best Results

1. **Start with a preset**: Choose the closest preset to your desired result
2. **Fine-tune**: Use the detailed controls to adjust the preset
3. **Test with your models**: Different models may look better with different presets
4. **Consider your audience**: Choose lighting that matches your presentation context

## Troubleshooting

If you don't see lighting changes:
1. Make sure you have 3D objects in your scene
2. Check that the Lighting Settings dialog shows "Applied" after clicking a preset
3. Try switching between different presets to see the difference
4. Ensure your graphics drivers support OpenGL lighting
5. Check the console/logs for lighting-related messages

## Technical Details

The lighting system works by:
1. **Configuration**: Each preset sets different light parameters
2. **Scene Update**: SceneManager receives the configuration changes
3. **Light Recreation**: Existing lights are removed and new ones created
4. **Rendering**: Coin3D renders the scene with new lighting
5. **Display**: Canvas refreshes to show the updated scene

## Advanced Usage

For more detailed control:
1. Use the "Environment Lighting" tab for ambient light adjustments
2. Use the "Light Management" tab for individual light control
3. Save custom configurations using the "Apply" and "OK" buttons
4. Experiment with different combinations of lights and settings 