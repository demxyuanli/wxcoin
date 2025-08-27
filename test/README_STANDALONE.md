# Standalone Docking Test Guide

This directory contains standalone test programs for the docking system that can be compiled and run without modifying the main project's CMakeLists.txt.

## Files

1. **standalone_docking_test.cpp** - Full-featured test application
2. **minimal_docking_example.cpp** - Minimal integration example
3. **build_standalone.sh** - Build script for Linux/macOS
4. **build_standalone.bat** - Build script for Windows

## Prerequisites

1. Build the main project first to create the docking library:
   ```bash
   cd /workspace
   mkdir build && cd build
   cmake ..
   make  # or cmake --build . --config Release on Windows
   ```

2. Ensure wxWidgets development packages are installed.

## Building and Running

### Linux/macOS

```bash
cd /workspace/test
./build_standalone.sh
./standalone_docking_test
```

Or manually:
```bash
g++ -o standalone_docking_test standalone_docking_test.cpp \
    -I../include `wx-config --cxxflags` \
    -L../build/src/docking -ldocking \
    `wx-config --libs` -std=c++17
```

### Windows

From Visual Studio Developer Command Prompt:
```cmd
cd \workspace\test
build_standalone.bat
standalone_docking_test.exe
```

## Features to Test

1. **Drag and Drop**
   - Drag tabs to rearrange panels
   - Drag tabs outside to create floating windows
   - Drag floating windows back to dock

2. **Auto-Hide**
   - Click pin button on panel title bars
   - Hover over auto-hidden tabs to show panels
   - Click outside to hide again

3. **Layout Management**
   - View menu → Save Layout
   - View menu → Load Layout
   - View menu → Reset Layout

4. **Perspectives**
   - View menu → Manage Perspectives
   - Save current layout as named perspective
   - Switch between saved perspectives

## Integration Guide

To integrate docking into your existing wxWidgets application:

1. **Include the docking library** in your project
2. **Replace wxPanel with DockManager** as your main container
3. **Convert existing panels to DockWidgets**
4. **Add the docking library to your link dependencies**

See `minimal_docking_example.cpp` for a simple integration example.

## Troubleshooting

### Build Errors

- **"docking library not found"**: Build the main project first
- **"wx-config not found"**: Install wxWidgets development packages
- **Linking errors**: Check library paths in build scripts

### Runtime Issues

- **Panels not visible**: Check if they're auto-hidden or closed
- **Can't drag panels**: Ensure DockWidget::DockWidgetMovable is set
- **Layout not saving**: Check file permissions

## Custom Modifications

To customize the test:

1. Edit `standalone_docking_test.cpp`
2. Rebuild using the build script
3. Test your changes

The test is self-contained and doesn't require CMakeLists.txt modifications.