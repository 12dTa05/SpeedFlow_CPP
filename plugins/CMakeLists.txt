cmake_minimum_required(VERSION 3.16)

# ============================================================================
# Custom GStreamer Plugin: speedcalc
# ============================================================================

add_library(gstspeedplugin SHARED
    gstspeedcalc.cpp
    homography.cpp
    speed_calculator.cpp
    plugin_register.cpp
)

target_include_directories(gstspeedplugin PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GSTREAMER_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    /opt/nvidia/deepstream/deepstream/sources/includes
)

target_link_libraries(gstspeedplugin
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${OpenCV_LIBS}
    nvdsgst_meta
    nvds_meta
)

# Install plugin to GStreamer plugin directory
install(TARGETS gstspeedplugin
    LIBRARY DESTINATION /usr/lib/x86_64-linux-gnu/gstreamer-1.0
)
