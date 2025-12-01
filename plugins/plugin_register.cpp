// plugin_register.cpp - Placeholder for Phase 2
// Will register custom GStreamer plugin

#include <gst/gst.h>

// Define PACKAGE for GStreamer plugin registration
#ifndef PACKAGE
#define PACKAGE "speedflow"
#endif

extern "C" {
    extern gboolean gst_speedcalc_plugin_init(GstPlugin* plugin);
    
    GST_PLUGIN_DEFINE(
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        speedplugin,
        "SpeedFlow custom GStreamer plugins",
        gst_speedcalc_plugin_init,
        "1.0.0",
        "Proprietary",
        "SpeedFlow",
        "https://github.com/speedflow"
    )
}
