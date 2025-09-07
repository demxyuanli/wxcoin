# RenderPreview System Testing Guide

## Overview

This guide explains how to test the enhanced RenderPreview system features, particularly the multi-light support and real-time updates.

## Testing Multi-Light Support

### 1. Basic Light Management

**Test Steps:**
1. Launch the application and open Render Preview dialog
2. Go to "Global Settings" tab
3. In the "Light List" section, verify you can:
   - Add new lights using "Add Light" button
   - Remove lights using "Remove Light" button
   - Select and edit light properties

**Expected Results:**
- Light list should show all configured lights
- Each light should have editable properties (name, type, position, direction, color, intensity)
- Changes should be reflected in the preview canvas

### 2. Light Presets Testing

**Test Steps:**
1. Go to "Light Presets" tab
2. Click on different preset buttons:
   - Studio Lighting
   - Outdoor Lighting
   - Dramatic Lighting
   - Warm Lighting
   - Cool Lighting
   - Minimal Lighting
   - FreeCAD Lighting
   - NavCube Lighting

**Expected Results:**
- Each preset should create different lighting configurations
- Preview canvas should immediately show:
  - Multiple light indicators (colored spheres) positioned according to light directions
  - Different lighting effects on the 3D objects
  - Changes in ambient, diffuse, and specular lighting

### 3. Visual Light Indicators

**Test Steps:**
1. Apply any light preset
2. Observe the preview canvas for light indicators

**Expected Results:**
- Each enabled light should have a visible indicator (colored sphere)
- Indicator color should match the light color
- Indicator position should reflect the light direction
- Multiple indicators should be visible for multiple lights

## Testing Real-Time Updates

### 1. Auto-Apply Mode

**Test Steps:**
1. Check the "Auto Apply" checkbox in the bottom toolbar
2. Make changes to lighting settings:
   - Change light intensity
   - Change light color
   - Add/remove lights
   - Change anti-aliasing settings
   - Change rendering mode

**Expected Results:**
- Changes should be immediately reflected in the preview canvas
- No need to click "Apply" button
- Real-time visual feedback

### 2. Manual Apply Mode

**Test Steps:**
1. Uncheck "Auto Apply" checkbox
2. Make changes to settings
3. Click "Apply" button

**Expected Results:**
- Changes should only be applied when "Apply" button is clicked
- Preview should update after clicking Apply

## Testing Configuration Validation

### 1. Invalid Settings

**Test Steps:**
1. Try to set invalid values:
   - Light intensity > 10.0
   - Invalid light type
   - Empty light name
   - Invalid position values

**Expected Results:**
- Validation error messages should appear
- Invalid settings should not be applied
- Preview should remain unchanged

### 2. Valid Settings

**Test Steps:**
1. Set valid values for all settings
2. Apply changes

**Expected Results:**
- No error messages
- Settings should be applied successfully
- Preview should update correctly

## Testing Undo/Redo Functionality

### 1. Basic Undo/Redo

**Test Steps:**
1. Make several changes to lighting settings
2. Click "Undo" button multiple times
3. Click "Redo" button multiple times

**Expected Results:**
- Undo should revert to previous states
- Redo should restore undone changes
- Preview should update accordingly
- Button states should reflect available operations

### 2. State Management

**Test Steps:**
1. Apply different light presets
2. Use Undo/Redo to navigate through states
3. Check if state descriptions are meaningful

**Expected Results:**
- States should be properly saved
- Navigation should work correctly
- State descriptions should be helpful

## Testing Multi-Light Rendering

### 1. Multiple Light Effects

**Test Steps:**
1. Create a scene with multiple lights:
   - Key light (white, high intensity)
   - Fill light (blue tint, medium intensity)
   - Rim light (warm color, low intensity)
2. Observe the combined lighting effect

**Expected Results:**
- Objects should show realistic multi-light illumination
- Shadows and highlights should be properly blended
- Each light should contribute to the final appearance

### 2. Light Interaction

**Test Steps:**
1. Enable/disable individual lights
2. Adjust light intensities
3. Change light colors

**Expected Results:**
- Lighting should change dynamically
- Effects should be cumulative
- No artifacts or rendering issues

## Performance Testing

### 1. Multiple Lights Performance

**Test Steps:**
1. Add 5-10 lights to the scene
2. Enable real-time updates
3. Make rapid changes to settings

**Expected Results:**
- Performance should remain acceptable
- No significant lag or freezing
- Smooth real-time updates

### 2. Memory Management

**Test Steps:**
1. Use undo/redo extensively
2. Switch between many different configurations
3. Monitor memory usage

**Expected Results:**
- Memory usage should be reasonable
- No memory leaks
- History should be properly managed

## Troubleshooting

### Common Issues

1. **Lights not visible in preview:**
   - Check if lights are enabled
   - Verify light intensity is not zero
   - Ensure light direction is valid

2. **No real-time updates:**
   - Verify "Auto Apply" is checked
   - Check if validation is blocking updates
   - Ensure event handlers are properly connected

3. **Undo/Redo not working:**
   - Verify changes were actually applied
   - Check if state was saved before changes
   - Ensure undo manager is properly initialized

### Debug Information

- Check application logs for error messages
- Verify all required components are loaded
- Test individual features in isolation

## Success Criteria

The RenderPreview system is working correctly if:

1. ✅ Multiple lights are properly rendered and visible
2. ✅ Light indicators show correct positions and colors
3. ✅ Real-time updates work smoothly
4. ✅ Configuration validation prevents invalid settings
5. ✅ Undo/Redo functionality works reliably
6. ✅ Performance remains acceptable with multiple lights
7. ✅ All preset configurations produce distinct lighting effects
8. ✅ No crashes or memory leaks occur during testing 