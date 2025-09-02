#!/bin/bash
# Clean and rebuild script

echo "Cleaning build directory..."
rm -rf build

echo "Creating new build directory..."
mkdir build
cd build

echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "Building..."
cmake --build . --config Release

echo "Build complete!"