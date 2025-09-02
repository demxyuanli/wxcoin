#!/bin/bash

# Build script for testing dock layout fix
echo "Building test application..."

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
make -j$(nproc)

# Run SimpleDockingFrame example
echo "Running SimpleDockingFrame test..."
./bin/SimpleDockingFrame

echo "Test complete!"