#ifndef PIPELINE_BUILDER_H
#define PIPELINE_BUILDER_H

#include <gst/gst.h>
#include <string>
#include "config_loader.h"

class PipelineBuilder {
public:
    PipelineBuilder(const PipelineConfig& config);
    ~PipelineBuilder();
    
    bool build(const std::string& source_uri);
    bool start();
    void stop();
    
    GstElement* getPipeline() { return pipeline_; }
    GstElement* getOsdElement() { return osd_; }
    
private:
    GstElement* buildSourceBin(const std::string& uri);
    GstElement* buildInferenceBin();
    GstElement* buildSinkBin();
    
    static void onPadAdded(GstElement* element, GstPad* pad, gpointer data);
    static GstPadProbeReturn busCallback(GstBus* bus, GstMessage* msg, gpointer data);
    
    PipelineConfig config_;
    GstElement* pipeline_;
    GstElement* source_;
    GstElement* muxer_;
    GstElement* pgie_;
    GstElement* tracker_;
    GstElement* analytics_;
    GstElement* osd_;
    GstElement* sink_;
    
    bool is_live_source_;
};

#endif // PIPELINE_BUILDER_H
