#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <vector>
#include <opencv2/core.hpp>

struct HomographyConfig {
    std::vector<cv::Point2f> source_points;
    std::vector<cv::Point2f> target_points;
    float target_width;
    float target_height;
    int config_width;   // Original resolution from YAML
    int config_height;
};

struct PipelineConfig {
    std::string infer_config_path;
    std::string tracker_config_path;
    std::string analytics_config_path;
    std::string homography_config_path;
    
    int muxer_width = 1280;
    int muxer_height = 720;
    int batch_size = 1;
    
    float video_fps = 25.0f;
    float speed_limit_kmh = 60.0f;
    
    // Validation thresholds
    int min_track_age_frames = 12;  // ~0.5s at 25fps
    float min_world_displ_m = 0.5f;
    float max_abs_kmh = 160.0f;
    float bbox_area_jump = 2.5f;
    float min_det_conf = 0.45f;
    int median_window = 5;
};

class ConfigLoader {
public:
    static PipelineConfig loadPipelineConfig(const std::string& yaml_path);
    static HomographyConfig loadHomographyConfig(const std::string& yaml_path, 
                                                  int muxer_width, 
                                                  int muxer_height);
private:
    static void scaleHomographyPoints(HomographyConfig& config, 
                                       int muxer_width, 
                                       int muxer_height);
};

#endif // CONFIG_LOADER_H
