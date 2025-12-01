// plugin_register.cpp - GStreamer plugin registration
// Registers custom SpeedFlow plugins

#include <gst/gst.h>

// Define PACKAGE for GStreamer plugin registration
#ifndef PACKAGE
#define PACKAGE "speedflow"
#endif

extern "C" {
    extern gboolean gst_speedcalc_plugin_init(GstPlugin* plugin);
    
    static gboolean plugin_init(GstPlugin* plugin) {
        // Register speedcalc element
        if (!gst_speedcalc_plugin_init(plugin)) {
            return FALSE;
        }
        
        return TRUE;
    }
    
    GST_PLUGIN_DEFINE(
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        speedplugin,
        "SpeedFlow custom GStreamer plugins",
        plugin_init,
        "1.0.0",
        "Proprietary",
        "SpeedFlow",
        "https://github.com/speedflow"
    )
}
