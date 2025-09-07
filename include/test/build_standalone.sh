#!/bin/bash

# Standalone build script for docking test
# This builds the test without modifying CMakeLists.txt

echo "Building standalone docking test..."

# Set paths
WORKSPACE_DIR=$(dirname $(dirname $(realpath $0)))
BUILD_DIR="$WORKSPACE_DIR/build"
TEST_DIR="$WORKSPACE_DIR/test"

# Check if main project is built
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Main project not built. Please build the main project first:"
    echo "  cd $WORKSPACE_DIR"
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  make"
    exit 1
fi

# Check if docking library exists
if [ ! -f "$BUILD_DIR/src/docking/libdocking.a" ] && [ ! -f "$BUILD_DIR/src/docking/libdocking.so" ]; then
    echo "Error: Docking library not found. Please build the main project first."
    exit 1
fi

# Detect wxWidgets configuration
WX_CONFIG=$(which wx-config)
if [ -z "$WX_CONFIG" ]; then
    echo "Error: wx-config not found. Please install wxWidgets development packages."
    exit 1
fi

# Get wxWidgets flags
WX_CXXFLAGS=$($WX_CONFIG --cxxflags)
WX_LIBS=$($WX_CONFIG --libs core,base)

# Compile standalone test
echo "Compiling standalone_docking_test.cpp..."
g++ -o "$TEST_DIR/standalone_docking_test" \
    "$TEST_DIR/standalone_docking_test.cpp" \
    -I"$WORKSPACE_DIR/include" \
    $WX_CXXFLAGS \
    -L"$BUILD_DIR/src/docking" \
    -ldocking \
    $WX_LIBS \
    -std=c++17 \
    -g

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Run with: $TEST_DIR/standalone_docking_test"
else
    echo "Build failed!"
    exit 1
fi
