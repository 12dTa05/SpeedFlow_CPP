#!/bin/bash
# Quick build script for SpeedFlow C++

set -e

echo "======================================"
echo "  SpeedFlow C++ Build Script"
echo "======================================"

# Check if build directory exists
if [ -d "build" ]; then
    echo "[INFO] Cleaning existing build directory..."
    rm -rf build
fi

# Create build directory
echo "[INFO] Creating build directory..."
mkdir build
cd build

# Run CMake
echo "[INFO] Running CMake..."
cmake ..

# Build
echo "[INFO] Building project..."
make -j$(nproc)

echo "======================================"
echo "  Build Complete!"
echo "======================================"
echo ""
echo "Run with:"
echo "  ./build/speedflow file:///path/to/video.mp4"
echo "  ./build/speedflow rtsp://camera_ip/stream"
echo ""
