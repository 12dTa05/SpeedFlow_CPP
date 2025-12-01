#include "config_loader.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <stdexcept>
#include <iostream>

PipelineConfig ConfigLoader::loadPipelineConfig(const std::string& yaml_path) {
    PipelineConfig config;
    
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        
        // Paths
        if (root["infer_config"]) {
            config.infer_config_path = root["infer_config"].as<std::string>();
        }
        if (root["tracker_config"]) {
            config.tracker_config_path = root["tracker_config"].as<std::string>();
        }
        if (root["analytics_config"]) {
            config.analytics_config_path = root["analytics_config"].as<std::string>();
        }
        if (root["homography_config"]) {
            config.homography_config_path = root["homography_config"].as<std::string>();
        }
        
        // Muxer settings
        if (root["muxer_width"]) {
            config.muxer_width = root["muxer_width"].as<int>();
        }
        if (root["muxer_height"]) {
            config.muxer_height = root["muxer_height"].as<int>();
        }
        if (root["batch_size"]) {
            config.batch_size = root["batch_size"].as<int>();
        }
        
        // Speed settings
        if (root["video_fps"]) {
            config.video_fps = root["video_fps"].as<float>();
        }
        if (root["speed_limit_kmh"]) {
            config.speed_limit_kmh = root["speed_limit_kmh"].as<float>();
        }
        
        // Validation thresholds
        if (root["min_track_age_frames"]) {
            config.min_track_age_frames = root["min_track_age_frames"].as<int>();
        }
        if (root["min_world_displ_m"]) {
            config.min_world_displ_m = root["min_world_displ_m"].as<float>();
        }
        if (root["max_abs_kmh"]) {
            config.max_abs_kmh = root["max_abs_kmh"].as<float>();
        }
        if (root["bbox_area_jump"]) {
            config.bbox_area_jump = root["bbox_area_jump"].as<float>();
        }
        if (root["min_det_conf"]) {
            config.min_det_conf = root["min_det_conf"].as<float>();
        }
        if (root["median_window"]) {
            config.median_window = root["median_window"].as<int>();
        }
        
        std::cout << "[ConfigLoader] Loaded pipeline config: " 
                  << config.muxer_width << "x" << config.muxer_height 
                  << " @ " << config.video_fps << " FPS" << std::endl;
        
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load pipeline config: " + std::string(e.what()));
    }
    
    return config;
}

HomographyConfig ConfigLoader::loadHomographyConfig(const std::string& yaml_path,
                                                     int muxer_width,
                                                     int muxer_height) {
    HomographyConfig config;
    
    try {
        YAML::Node root = YAML::LoadFile(yaml_path);
        
        // Load source points
        if (root["SOURCE"]) {
            auto source_node = root["SOURCE"];
            for (const auto& point : source_node) {
                float x = point[0].as<float>();
                float y = point[1].as<float>();
                config.source_points.push_back(cv::Point2f(x, y));
            }
        }
        
        // Load target points
        if (root["TARGET"]) {
            auto target_node = root["TARGET"];
            for (const auto& point : target_node) {
                float x = point[0].as<float>();
                float y = point[1].as<float>();
                config.target_points.push_back(cv::Point2f(x, y));
            }
        }
        
        // Load target dimensions
        if (root["TARGET_WIDTH"]) {
            config.target_width = root["TARGET_WIDTH"].as<float>();
        }
        if (root["TARGET_HEIGHT"]) {
            config.target_height = root["TARGET_HEIGHT"].as<float>();
        }
        
        // Detect original config resolution from analytics config
        // For now, assume 1280x720 as default (can be read from analytics config)
        config.config_width = 1280;
        config.config_height = 720;
        
        // CRITICAL: Scale homography points to match muxer resolution
        scaleHomographyPoints(config, muxer_width, muxer_height);
        
        std::cout << "[ConfigLoader] Loaded homography config with " 
                  << config.source_points.size() << " points (scaled to "
                  << muxer_width << "x" << muxer_height << ")" << std::endl;
        
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load homography config: " + std::string(e.what()));
    }
    
    return config;
}

void ConfigLoader::scaleHomographyPoints(HomographyConfig& config,
                                          int muxer_width,
                                          int muxer_height) {
    float scale_x = static_cast<float>(muxer_width) / config.config_width;
    float scale_y = static_cast<float>(muxer_height) / config.config_height;
    
    std::cout << "[ConfigLoader] Scaling homography points: "
              << "scale_x=" << scale_x << ", scale_y=" << scale_y << std::endl;
    
    for (auto& pt : config.source_points) {
        pt.x *= scale_x;
        pt.y *= scale_y;
    }
}
