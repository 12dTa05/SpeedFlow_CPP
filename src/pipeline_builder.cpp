#include "pipeline_builder.h"
#include <iostream>
#include <cstring>

#define CHECK_ELEMENT(elem, name) \
    if (!elem) { \
        std::cerr << "Failed to create element: " << name << std::endl; \
        return false; \
    }

#define CHECK_ELEMENT_PTR(elem, name) \
    if (!elem) { \
        std::cerr << "Failed to create element: " << name << std::endl; \
        return nullptr; \
    }

PipelineBuilder::PipelineBuilder(const PipelineConfig& config)
    : config_(config),
      pipeline_(nullptr),
      source_(nullptr),
      muxer_(nullptr),
      pgie_(nullptr),
      tracker_(nullptr),
      analytics_(nullptr),
      osd_(nullptr),
      sink_(nullptr),
      is_live_source_(false) {
}

PipelineBuilder::~PipelineBuilder() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
    }
}

bool PipelineBuilder::build(const std::string& source_uri) {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
    
    // Create pipeline
    pipeline_ = gst_pipeline_new("speedflow-pipeline");
    if (!pipeline_) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return false;
    }
    
    // Detect source type
    is_live_source_ = (source_uri.find("rtsp://") == 0);
    
    // Build components
    source_ = buildSourceBin(source_uri);
    if (!source_) return false;
    
    muxer_ = gst_element_factory_make("nvstreammux", "stream-muxer");
    CHECK_ELEMENT(muxer_, "nvstreammux");
    
    pgie_ = gst_element_factory_make("nvinfer", "primary-infer");
    CHECK_ELEMENT(pgie_, "nvinfer");
    
    tracker_ = gst_element_factory_make("nvtracker", "tracker");
    CHECK_ELEMENT(tracker_, "nvtracker");
    
    analytics_ = gst_element_factory_make("nvdsanalytics", "analytics");
    CHECK_ELEMENT(analytics_, "nvdsanalytics");
    
    osd_ = gst_element_factory_make("nvdsosd", "onscreendisplay");
    CHECK_ELEMENT(osd_, "nvdsosd");
    
    sink_ = buildSinkBin();
    if (!sink_) return false;
    
    // Configure elements
    g_object_set(G_OBJECT(muxer_),
                 "batch-size", config_.batch_size,
                 "width", config_.muxer_width,
                 "height", config_.muxer_height,
                 "batched-push-timeout", 40000,
                 "live-source", is_live_source_ ? 1 : 0,
                 nullptr);
    
    g_object_set(G_OBJECT(pgie_),
                 "config-file-path", config_.infer_config_path.c_str(),
                 nullptr);
    
    g_object_set(G_OBJECT(tracker_),
                 "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
                 "ll-config-file", config_.tracker_config_path.c_str(),
                 "tracker-width", 640,
                 "tracker-height", 384,
                 "gpu-id", 0,
                 nullptr);
    
    g_object_set(G_OBJECT(analytics_),
                 "config-file", config_.analytics_config_path.c_str(),
                 nullptr);
    
    g_object_set(G_OBJECT(osd_),
                 "display-text", 1,
                 "display-bbox", 1,
                 nullptr);
    
    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(pipeline_), source_, muxer_, pgie_, tracker_,
                     analytics_, osd_, sink_, nullptr);
    
    // Link static pads
    if (!gst_element_link_many(muxer_, pgie_, tracker_, analytics_, osd_, sink_, nullptr)) {
        std::cerr << "Failed to link pipeline elements" << std::endl;
        return false;
    }
    
    // Connect pad-added signal for dynamic source linking
    g_signal_connect(source_, "pad-added", G_CALLBACK(onPadAdded), muxer_);
    
    // Setup bus watch
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    gst_bus_add_watch(bus, (GstBusFunc)busCallback, this);
    gst_object_unref(bus);
    
    std::cout << "[PipelineBuilder] Pipeline built successfully" << std::endl;
    return true;
}

GstElement* PipelineBuilder::buildSourceBin(const std::string& uri) {
    GstElement* source = gst_element_factory_make("uridecodebin", "source-bin");
    CHECK_ELEMENT_PTR(source, "uridecodebin");
    
    g_object_set(G_OBJECT(source), "uri", uri.c_str(), nullptr);
    
    // Configure RTSP source if applicable
    auto on_source_setup = +[](GstElement* decodebin, GstElement* src, gpointer user_data) {
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(src), "latency")) {
            g_object_set(G_OBJECT(src), "latency", 100, nullptr);
        }
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(src), "drop-on-latency")) {
            g_object_set(G_OBJECT(src), "drop-on-latency", TRUE, nullptr);
        }
    };
    
    g_signal_connect(source, "source-setup", G_CALLBACK(on_source_setup), nullptr);
    
    std::cout << "[PipelineBuilder] Source configured: " << uri << std::endl;
    return source;
}

GstElement* PipelineBuilder::buildSinkBin() {
    // MJPEG HTTP Sink for headless debugging
    // Allows viewing stream at http://<device-ip>:8080
    
    GstElement* conv = gst_element_factory_make("nvvideoconvert", "conv");
    CHECK_ELEMENT_PTR(conv, "nvvideoconvert");
    
    // Convert to software format (I420) for jpegenc
    GstElement* sw_conv = gst_element_factory_make("videoconvert", "sw_conv");
    CHECK_ELEMENT_PTR(sw_conv, "videoconvert");

    GstElement* jpegenc = gst_element_factory_make("jpegenc", "jpegenc");
    CHECK_ELEMENT_PTR(jpegenc, "jpegenc");
    
    GstElement* multipart = gst_element_factory_make("multipartmux", "multipart");
    CHECK_ELEMENT_PTR(multipart, "multipartmux");
    
    GstElement* sink = gst_element_factory_make("tcpserversink", "mjpeg-sink");
    CHECK_ELEMENT_PTR(sink, "tcpserversink");
    
    g_object_set(G_OBJECT(sink), "host", "0.0.0.0", "port", 8080, nullptr);
    
    // Create bin
    GstElement* bin = gst_bin_new("sink-bin");
    gst_bin_add_many(GST_BIN(bin), conv, sw_conv, jpegenc, multipart, sink, nullptr);
    
    if (!gst_element_link_many(conv, sw_conv, jpegenc, multipart, sink, nullptr)) {
        std::cerr << "[PipelineBuilder] Failed to link MJPEG sink elements" << std::endl;
        return nullptr;
    }
    
    // Add ghost pad
    GstPad* pad = gst_element_get_static_pad(conv, "sink");
    GstPad* ghost_pad = gst_ghost_pad_new("sink", pad);
    gst_pad_set_active(ghost_pad, TRUE);
    gst_element_add_pad(bin, ghost_pad);
    gst_object_unref(pad);
    
    std::cout << "[PipelineBuilder] Sink bin created (MJPEG HTTP mode)" << std::endl;
    std::cout << "[PipelineBuilder] View stream at: http://<device-ip>:8080" << std::endl;
    return bin;
}

void PipelineBuilder::onPadAdded(GstElement* element, GstPad* pad, gpointer data) {
    GstElement* muxer = static_cast<GstElement*>(data);
    
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) return;
    
    const gchar* name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    
    if (g_str_has_prefix(name, "video/")) {
        GstPad* sinkpad = gst_element_request_pad_simple(muxer, "sink_0");
        if (sinkpad && !gst_pad_is_linked(sinkpad)) {
            if (gst_pad_link(pad, sinkpad) == GST_PAD_LINK_OK) {
                std::cout << "[PipelineBuilder] Source pad linked to muxer" << std::endl;
            }
            gst_object_unref(sinkpad);
        }
    }
    
    gst_caps_unref(caps);
}

GstPadProbeReturn PipelineBuilder::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            std::cout << "[Pipeline] End of stream" << std::endl;
            break;
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug;
            gst_message_parse_error(msg, &err, &debug);
            std::cerr << "[Pipeline] Error: " << err->message << std::endl;
            if (debug) {
                std::cerr << "[Pipeline] Debug: " << debug << std::endl;
            }
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError* err;
            gchar* debug;
            gst_message_parse_warning(msg, &err, &debug);
            std::cout << "[Pipeline] Warning: " << err->message << std::endl;
            g_error_free(err);
            g_free(debug);
            break;
        }
        default:
            break;
    }
    return GST_PAD_PROBE_OK;
}

bool PipelineBuilder::start() {
    if (!pipeline_) {
        std::cerr << "Pipeline not built" << std::endl;
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start pipeline" << std::endl;
        return false;
    }
    
    std::cout << "[PipelineBuilder] Pipeline started" << std::endl;
    return true;
}

void PipelineBuilder::stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        std::cout << "[PipelineBuilder] Pipeline stopped" << std::endl;
    }
}
