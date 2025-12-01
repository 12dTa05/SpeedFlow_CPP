// gstspeedcalc.cpp - Custom GStreamer plugin for speed calculation
// Integrates with DeepStream metadata pipeline

#include <string>
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gstnvdsmeta.h"
#include "nvdsmeta.h"
#include "nvds_analytics_meta.h"

#include "homography.h"
#include "speed_calculator.h"
#include <memory>
#include <iostream>

GST_DEBUG_CATEGORY_STATIC(gst_speedcalc_debug);
#define GST_CAT_DEFAULT gst_speedcalc_debug

#define GST_TYPE_SPEEDCALC (gst_speedcalc_get_type())
#define GST_SPEEDCALC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SPEEDCALC, GstSpeedCalc))
#define GST_SPEEDCALC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SPEEDCALC, GstSpeedCalcClass))

typedef struct _GstSpeedCalc GstSpeedCalc;
typedef struct _GstSpeedCalcClass GstSpeedCalcClass;

struct _GstSpeedCalc {
    GstBaseTransform parent;
    
    // Speed calculator instance
    std::shared_ptr<speedflow::SpeedCalculator> calculator;
    
    // Configuration
    gint muxer_width;
    gint muxer_height;
};

struct _GstSpeedCalcClass {
    GstBaseTransformClass parent_class;
};

GType gst_speedcalc_get_type(void);

// Properties
enum {
    PROP_0,
    PROP_CALCULATOR,
    PROP_MUXER_WIDTH,
    PROP_MUXER_HEIGHT
};

// Function declarations
static void gst_speedcalc_set_property(GObject* object, guint prop_id,
                                       const GValue* value, GParamSpec* pspec);
static void gst_speedcalc_get_property(GObject* object, guint prop_id,
                                       GValue* value, GParamSpec* pspec);
static GstFlowReturn gst_speedcalc_transform_ip(GstBaseTransform* trans,
                                                GstBuffer* buf);
static void gst_speedcalc_finalize(GObject* object);

// GStreamer boilerplate
#define gst_speedcalc_parent_class parent_class
G_DEFINE_TYPE(GstSpeedCalc, gst_speedcalc, GST_TYPE_BASE_TRANSFORM);

// Pad templates
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY
);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY
);

static void gst_speedcalc_class_init(GstSpeedCalcClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    GstBaseTransformClass* transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    
    gobject_class->set_property = gst_speedcalc_set_property;
    gobject_class->get_property = gst_speedcalc_get_property;
    gobject_class->finalize = gst_speedcalc_finalize;
    
    transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_speedcalc_transform_ip);
    
    // Add pad templates
    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);
    
    // Properties
    g_object_class_install_property(gobject_class, PROP_CALCULATOR,
        g_param_spec_pointer("calculator", "Speed Calculator",
            "Pointer to SpeedCalculator instance",
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_MUXER_WIDTH,
        g_param_spec_int("muxer-width", "Muxer Width",
            "Width of muxer output", 0, G_MAXINT, 1280,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    g_object_class_install_property(gobject_class, PROP_MUXER_HEIGHT,
        g_param_spec_int("muxer-height", "Muxer Height",
            "Height of muxer output", 0, G_MAXINT, 720,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    
    gst_element_class_set_static_metadata(element_class,
        "Speed Calculator",
        "Filter/Metadata",
        "Calculates vehicle speed from tracking data",
        "SpeedFlow Team");
    
    GST_DEBUG_CATEGORY_INIT(gst_speedcalc_debug, "speedcalc", 0,
        "Speed calculation plugin");
}

static void gst_speedcalc_init(GstSpeedCalc* speedcalc) {
    speedcalc->calculator = nullptr;
    speedcalc->muxer_width = 1280;
    speedcalc->muxer_height = 720;
    
    // Set passthrough mode (we only modify metadata, not buffer data)
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(speedcalc), TRUE);
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(speedcalc), TRUE);
}

static void gst_speedcalc_set_property(GObject* object, guint prop_id,
                                       const GValue* value, GParamSpec* pspec) {
    GstSpeedCalc* speedcalc = GST_SPEEDCALC(object);
    
    switch (prop_id) {
        case PROP_CALCULATOR:
            speedcalc->calculator = *static_cast<std::shared_ptr<speedflow::SpeedCalculator>*>(
                g_value_get_pointer(value));
            break;
        case PROP_MUXER_WIDTH:
            speedcalc->muxer_width = g_value_get_int(value);
            break;
        case PROP_MUXER_HEIGHT:
            speedcalc->muxer_height = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_speedcalc_get_property(GObject* object, guint prop_id,
                                       GValue* value, GParamSpec* pspec) {
    GstSpeedCalc* speedcalc = GST_SPEEDCALC(object);
    
    switch (prop_id) {
        case PROP_MUXER_WIDTH:
            g_value_set_int(value, speedcalc->muxer_width);
            break;
        case PROP_MUXER_HEIGHT:
            g_value_set_int(value, speedcalc->muxer_height);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static GstFlowReturn gst_speedcalc_transform_ip(GstBaseTransform* trans,
                                                GstBuffer* buf) {
    GstSpeedCalc* speedcalc = GST_SPEEDCALC(trans);
    
    if (!speedcalc->calculator) {
        GST_WARNING_OBJECT(speedcalc, "Speed calculator not initialized");
        return GST_FLOW_OK;
    }
    
    // Access DeepStream batch metadata
    NvDsBatchMeta* batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    if (!batch_meta) {
        return GST_FLOW_OK;
    }
    
    // Iterate through frames in batch
    for (NvDsMetaList* l_frame = batch_meta->frame_meta_list; l_frame != NULL;
         l_frame = l_frame->next) {
        NvDsFrameMeta* frame_meta = (NvDsFrameMeta*)(l_frame->data);
        
        // Iterate through objects in frame
        for (NvDsMetaList* l_obj = frame_meta->obj_meta_list; l_obj != NULL;
             l_obj = l_obj->next) {
            NvDsObjectMeta* obj_meta = (NvDsObjectMeta*)(l_obj->data);
            
            // Skip if not tracked or not a vehicle
            if (obj_meta->object_id == UNTRACKED_OBJECT_ID) {
                continue;
            }
            
            // Calculate center-bottom point
            float cx = obj_meta->rect_params.left + obj_meta->rect_params.width / 2.0f;
            float bottom_y = obj_meta->rect_params.top + obj_meta->rect_params.height;
            float bbox_area = obj_meta->rect_params.width * obj_meta->rect_params.height;
            float det_conf = obj_meta->confidence;
            
            // Process with speed calculator
            auto measurement = speedcalc->calculator->processObject(
                obj_meta->object_id,
                cx,
                bottom_y,
                bbox_area,
                det_conf,
                frame_meta->frame_num
            );
            
            // If valid measurement, update display text
            if (measurement.is_valid) {
                std::string speed_text = speedcalc->calculator->getSpeedText(obj_meta->object_id);
                
                // Update object text display
                if (obj_meta->text_params.display_text) {
                    g_free(obj_meta->text_params.display_text);
                }
                obj_meta->text_params.display_text = g_strdup(speed_text.c_str());
                
                // Log overspeeding
                if (measurement.is_overspeeding) {
                    GST_INFO_OBJECT(speedcalc, "Overspeed detected: Track %d @ %.1f km/h",
                                   measurement.track_id, measurement.speed_kmh);
                }
            }
        }
    }
    
    return GST_FLOW_OK;
}

static void gst_speedcalc_finalize(GObject* object) {
    GstSpeedCalc* speedcalc = GST_SPEEDCALC(object);
    speedcalc->calculator.reset();
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

// Plugin registration
extern "C" {
    gboolean gst_speedcalc_plugin_init(GstPlugin* plugin) {
        return gst_element_register(plugin, "speedcalc", GST_RANK_NONE,
                                    GST_TYPE_SPEEDCALC);
    }
}
