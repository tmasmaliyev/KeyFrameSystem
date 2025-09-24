#!/bin/bash

# Build script for Key-framing Motion Control System
# This script automates the build process using CMake

set -e  # Exit on any error

PROJECT_NAME="KeyframeMotionSystem"
BUILD_DIR="build"

echo "Building $PROJECT_NAME..."

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed. Please run the setup script first:"
    echo "  chmod +x setup.sh"
    echo "  ./setup.sh"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Run CMake configuration
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building project..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "To run the application:"
    echo "  cd build"
    echo "  ./keyframe_system"
    echo ""
    echo "Controls:"
    echo "  Q - Toggle between Quaternions and Euler Angles"
    echo "  S - Toggle between B-Spline and Catmull-Rom interpolation"
    echo "  R - Reset animation"
    echo "  ESC - Exit"
else
    echo "✗ Build failed!"
    exit 1
fi