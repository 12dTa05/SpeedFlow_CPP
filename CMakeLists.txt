cmake_minimum_required(VERSION 3.16)
project(SpeedFlow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# Find Required Packages
# ============================================================================

# DeepStream & GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)

# OpenCV
find_package(OpenCV REQUIRED)

# Protobuf
find_package(Protobuf REQUIRED)

# yaml-cpp
find_package(yaml-cpp REQUIRED)

# Oat++
find_package(oatpp REQUIRED)
find_package(oatpp-websocket REQUIRED)

# CUDA (for DeepStream)
find_package(CUDA REQUIRED)

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/plugins
    ${GSTREAMER_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
    ${CUDA_INCLUDE_DIRS}
    /opt/nvidia/deepstream/deepstream/sources/includes
    /usr/local/include  # For concurrentqueue
)

link_directories(
    ${GSTREAMER_LIBRARY_DIRS}
    /opt/nvidia/deepstream/deepstream/lib
)

# ============================================================================
# Protobuf Generation
# ============================================================================

set(PROTO_FILES ${CMAKE_SOURCE_DIR}/proto/speedflow.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# ============================================================================
# Source Files
# ============================================================================

set(SPEEDFLOW_SOURCES
    src/main.cpp
    src/pipeline_builder.cpp
    src/config_loader.cpp
    src/api_server.cpp
    plugins/homography.cpp
    plugins/speed_calculator.cpp
    ${PROTO_SRCS}
)

# ============================================================================
# Main Executable
# ============================================================================

add_executable(speedflow ${SPEEDFLOW_SOURCES})

target_link_libraries(speedflow
    ${GSTREAMER_LIBRARIES}
    ${OpenCV_LIBS}
    ${Protobuf_LIBRARIES}
    yaml-cpp
    oatpp::oatpp
    oatpp::oatpp-websocket
    nvdsgst_meta
    nvds_meta
    nvbufsurface
    nvbufsurftransform
    pthread
)

# ============================================================================
# Custom GStreamer Plugin
# ============================================================================

add_subdirectory(plugins)

# ============================================================================
# Frontend Build (Optional - run manually)
# ============================================================================

add_custom_target(frontend
    COMMAND npm install
    COMMAND npm run build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/frontend
    COMMENT "Building React frontend..."
)

add_custom_target(copy_frontend
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/frontend/dist
        ${CMAKE_BINARY_DIR}/static
    DEPENDS frontend
    COMMENT "Copying frontend build to static directory..."
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS speedflow DESTINATION bin)
install(DIRECTORY configs/ DESTINATION share/speedflow/configs)

# ============================================================================
# Testing (Optional)
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
