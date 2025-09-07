# Docking System Test Programs

This directory contains test programs for the wxWidgets Advanced Docking System.

## Test Programs

### 1. Simple Test (`simple_test.cpp`)
A minimal example showing basic docking functionality:
- Creates a dock manager
- Adds three dock widgets (editor, project tree, output)
- Demonstrates basic docking areas

### 2. Full Test Application (`docking_test_app.cpp`)
A comprehensive test application with:
- Menu and toolbar
- Multiple dock widget types (editors, tools, properties)
- Save/load layout functionality
- Perspective management
- Auto-hide testing
- Floating window support

### 3. Automated Feature Test (`test_all_features.cpp`)
An automated test suite that exercises:
- All docking positions
- Tabbed docking
- Floating windows
- Auto-hide functionality
- Perspectives
- Splitter functionality
- Drag and drop
- State persistence
- Edge cases

## Building the Tests

From the main project directory:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release

# The test executables will be in build/test/
```

## Running the Tests

### Windows:
```bash
cd build\test\Release
docking_test.exe
```

### Linux/macOS:
```bash
cd build/test
./docking_test
```

## Test Features

### Basic Docking
- Dock widgets to any side (left, right, top, bottom, center)
- Split areas horizontally or vertically
- Resize dock areas using splitters

### Tabbing
- Drop widgets onto existing dock areas to create tabs
- Switch between tabs
- Close individual tabs
- Reorder tabs by dragging

### Floating
- Undock widgets to create floating windows
- Redock floating windows by dragging
- Multiple widgets in floating containers

### Auto-Hide
- Pin/unpin widgets to auto-hide
- Hover over auto-hide tabs to show widgets
- Click outside to hide again

### Perspectives
- Save current layout as a perspective
- Switch between saved perspectives
- Manage perspectives (rename, delete, export)
- Auto-save perspective option

### State Persistence
- Save layout to XML
- Load layout from XML
- Preserves widget states and positions

## Customization

The test applications demonstrate how to:
- Create custom dock widgets
- Set widget features (closable, movable, floatable)
- Configure dock manager behavior
- Handle docking events
- Integrate with existing applications
