#ifndef PIPELINE_BUILDER_H
#define PIPELINE_BUILDER_H

#include <gst/gst.h>
#include <string>
#include <memory>
#include "config_loader.h"
#include "../plugins/speed_calculator.h"

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
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);
    
    PipelineConfig config_;
    GstElement* pipeline_;
    GstElement* source_;
    GstElement* muxer_;
    GstElement* pgie_;
    GstElement* tracker_;
    GstElement* analytics_;
    GstElement* speedcalc_;  // Custom speed calculation plugin
    GstElement* osd_;
    GstElement* sink_;
    
    bool is_live_source_;
    std::shared_ptr<speedflow::SpeedCalculator> speed_calculator_;
};

#endif // PIPELINE_BUILDER_H
