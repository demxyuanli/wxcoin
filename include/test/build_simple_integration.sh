#!/bin/bash

# Build script for simple docking integration test
# This builds without complex dependencies

echo "Building simple docking integration test..."

# Set paths
WORKSPACE_DIR=$(dirname $(dirname $(realpath $0)))
BUILD_DIR="$WORKSPACE_DIR/build"
TEST_DIR="$WORKSPACE_DIR/test"

# Check if docking library exists
if [ ! -f "$BUILD_DIR/src/docking/libdocking.a" ] && [ ! -f "$BUILD_DIR/src/docking/libdocking.so" ]; then
    echo "Error: Docking library not found. Build the main project first."
    exit 1
fi

# Get wxWidgets flags
WX_CXXFLAGS=$(wx-config --cxxflags)
WX_LIBS=$(wx-config --libs core,base,stc)

# Compile simple integration test
echo "Compiling simple_docking_integration.cpp..."
g++ -o "$TEST_DIR/simple_docking_integration" \
    "$TEST_DIR/simple_docking_integration.cpp" \
    -I"$WORKSPACE_DIR/include" \
    $WX_CXXFLAGS \
    -L"$BUILD_DIR/src/docking" \
    -ldocking \
    $WX_LIBS \
    -std=c++17 \
    -g

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "Run with: $TEST_DIR/simple_docking_integration"
    echo ""
    echo "Features to test:"
    echo "- Drag panels to rearrange"
    echo "- Create floating windows"
    echo "- Pin/unpin panels for auto-hide"
    echo "- Save and manage perspectives"
else
    echo "Build failed!"
    exit 1
fi
