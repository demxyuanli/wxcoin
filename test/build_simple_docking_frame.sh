#!/bin/bash

# Build script for SimpleDockingFrame

echo "Building SimpleDockingFrame..."

# Set paths
WORKSPACE_DIR=$(dirname $(dirname $(realpath $0)))
BUILD_DIR="$WORKSPACE_DIR/build"
SRC_DIR="$WORKSPACE_DIR/src/ui"
TEST_DIR="$WORKSPACE_DIR/test"

# Check if docking library exists
if [ ! -f "$BUILD_DIR/src/docking/libdocking.a" ] && [ ! -f "$BUILD_DIR/src/docking/libdocking.so" ]; then
    echo "Error: Docking library not found. Build the main project first."
    exit 1
fi

# Get wxWidgets flags
WX_CXXFLAGS=$(wx-config --cxxflags)
WX_LIBS=$(wx-config --libs core,base)

# Get all required libraries
LIBS="-L$BUILD_DIR/src/docking -ldocking"
LIBS="$LIBS -L$BUILD_DIR/src/view -lCADView"
LIBS="$LIBS -L$BUILD_DIR/src/ui -lUIPanelProperty -lUIPanelTree"
LIBS="$LIBS -L$BUILD_DIR/src/opencascade -lCADOCC"
LIBS="$LIBS -L$BUILD_DIR/src/core -lCADCore"
LIBS="$LIBS -L$BUILD_DIR/src/config -lCADConfig"
LIBS="$LIBS -L$BUILD_DIR/src/logger -lCADLogger"
LIBS="$LIBS -L$BUILD_DIR/src/rendering -lCADRendering"
LIBS="$LIBS -L$BUILD_DIR/src/widgets -lwidgets"

# Add OpenCASCADE and Coin3D
LIBS="$LIBS -lTKernel -lTKMath -lTKBRep -lTKG3d -lTKG2d -lTKGeomBase -lTKTopAlgo"
LIBS="$LIBS -lCoin -lSoQt"

# Compile
echo "Compiling SimpleDockingFrame.cpp..."
g++ -o "$TEST_DIR/simple_docking_frame" \
    "$SRC_DIR/SimpleDockingFrame.cpp" \
    -I"$WORKSPACE_DIR/include" \
    $WX_CXXFLAGS \
    $LIBS \
    $WX_LIBS \
    -std=c++17 \
    -g

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Run with: $TEST_DIR/simple_docking_frame"
else
    echo "Build failed!"
    exit 1
fi