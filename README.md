# SpeedFlow C++ - Traffic Monitoring System

High-performance traffic speed monitoring system built with C++, DeepStream 7.1, and WebRTC streaming.

## Overview

This is a production-grade C++ implementation migrated from the Python prototype in `IoT_Graduate/`. The system runs on NVIDIA Jetson Orin with JetPack 6.2 and provides real-time vehicle speed detection with sub-500ms latency.

**Key Features:**
- ✅ DeepStream 7.1 pipeline with YOLO11 inference
- ✅ Custom GStreamer plugin for speed calculation
- ✅ Homography-based perspective transformation
- ✅ WebRTC video streaming (low latency)
- ✅ Oat++ WebSocket API for real-time data
- ✅ React frontend with Canvas overlay

## Project Structure

```
SpeedFlow_CPP/
├── CMakeLists.txt              # Root build configuration
├── src/
│   ├── main.cpp                # Entry point
│   ├── pipeline_builder.cpp    # GStreamer pipeline management
│   ├── config_loader.cpp       # YAML config parser
│   └── api_server.cpp          # Oat++ WebSocket server (Phase 3)
├── plugins/
│   ├── gstspeedcalc.cpp        # Custom speed calculation plugin (Phase 2)
│   ├── homography.cpp          # Perspective transformation (Phase 2)
│   └── plugin_register.cpp     # GStreamer plugin registration
├── proto/
│   └── speedflow.proto         # Protobuf schema (Phase 3)
├── configs/
│   ├── pipeline.yml            # Main configuration
│   ├── config_infer_primary_yolo11.txt
│   ├── config_nvdsanalytics.txt
│   └── points_source_target.yml
├── frontend/                   # React app (Phase 4)
└── tests/                      # Unit tests (Phase 5)
```

## Prerequisites

### Hardware
- NVIDIA Jetson Orin (or compatible)
- JetPack 6.2 installed

### Software
- DeepStream SDK 7.1
- CUDA 12.6
- TensorRT 10.3
- GStreamer 1.20+
- OpenCV 4.x
- yaml-cpp
- Oat++ (will install below)
- Protobuf compiler

## Installation

### 1. Install System Dependencies

```bash
sudo apt update
sudo apt install -y \
    build-essential cmake git \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libopencv-dev libyaml-cpp-dev \
    protobuf-compiler libprotobuf-dev \
    libglib2.0-dev
```

### 2. Install Oat++ Framework

```bash
# Oat++ core
git clone https://github.com/oatpp/oatpp.git
cd oatpp && mkdir build && cd build
cmake .. && make -j$(nproc) && sudo make install

# Oat++ WebSocket module
cd ../..
git clone https://github.com/oatpp/oatpp-websocket.git
cd oatpp-websocket && mkdir build && cd build
cmake .. && make -j$(nproc) && sudo make install
```

### 3. Install Lock-Free Queue (Header-Only)

```bash
git clone https://github.com/cameron314/concurrentqueue.git
sudo cp concurrentqueue/*.h /usr/local/include/
```

### 4. Build DeepStream-Yolo Custom Parser

```bash
cd /path/to/DeepStream-Yolo
export CUDA_VER=12.6
make -C nvdsinfer_custom_impl_Yolo clean
make -C nvdsinfer_custom_impl_Yolo
```

## Build Instructions

### Phase 1 (Current) - Basic Pipeline

```bash
cd SpeedFlow_CPP
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run with Display Sink (Testing)

```bash
# Test with file
./speedflow file:///path/to/video.mp4

# Test with RTSP stream
./speedflow rtsp://192.168.1.100/stream --config ../configs/pipeline.yml
```

**Expected Output:**
- GStreamer pipeline starts
- Video displays on screen (nveglglessink)
- Inference + tracking working
- Press Ctrl+C to stop gracefully

## Configuration

Edit `configs/pipeline.yml` to customize:

```yaml
# Muxer resolution (CRITICAL: must match homography calibration)
muxer_width: 1280
muxer_height: 720

# Speed detection
speed_limit_kmh: 60.0
video_fps: 25.0

# Validation thresholds
min_track_age_frames: 12    # Ignore new tracks
min_world_displ_m: 0.5      # Minimum movement
max_abs_kmh: 160.0          # Physical limit
```

## Development Roadmap

- [x] **Phase 1:** Foundation (CMake, pipeline builder, config loader) ✅ **COMPLETE**
- [ ] **Phase 2:** Custom Speed Plugin (homography, speed calculation)
- [ ] **Phase 3:** API Layer (Protobuf, WebSocket, WebRTC signaling)
- [ ] **Phase 4:** Frontend (React, WebRTC player, Canvas overlay)
- [ ] **Phase 5:** Optimization (stress testing, memory leak checks)

## Critical Implementation Notes

### 1. Resolution Scaling
The homography points from `points_source_target.yml` are automatically scaled to match the muxer resolution. This is handled in `config_loader.cpp`:

```cpp
float scale_x = muxer_width / config_width;
float scale_y = muxer_height / config_height;
// Apply to all source points
```

### 2. Memory Management
- Use `g_autoptr()` for GStreamer objects
- Use `std::shared_ptr` for custom classes
- Run `valgrind` before deployment

### 3. Threading Model
- GStreamer probe callbacks must be non-blocking
- Heavy I/O goes to worker threads via lock-free queue

## Troubleshooting

### Build Errors

**Error:** `Could not find oatpp`
```bash
# Make sure oatpp is installed to /usr/local
sudo ldconfig
```

**Error:** `nvdsgst_meta not found`
```bash
# Add DeepStream lib path
export LD_LIBRARY_PATH=/opt/nvidia/deepstream/deepstream/lib:$LD_LIBRARY_PATH
```

### Runtime Errors

**Error:** `Failed to create element: nvinfer`
```bash
# Check DeepStream installation
gst-inspect-1.0 nvinfer
```

**Error:** `Pipeline stuck at PAUSED`
- Check if video file exists
- For RTSP: verify network connectivity
- Check GStreamer debug: `GST_DEBUG=3 ./speedflow <uri>`

## License

Proprietary - SpeedFlow Traffic Monitoring System

## References

- [DeepStream SDK Documentation](https://docs.nvidia.com/metropolis/deepstream/dev-guide/)
- [Oat++ Documentation](https://oatpp.io/docs/start/)
- [Implementation Plan](../brain/implementation_plan.md)
