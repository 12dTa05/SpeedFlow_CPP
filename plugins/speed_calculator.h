#pragma once

#include "homography.h"
#include <deque>
#include <unordered_map>
#include <memory>
#include <string>

namespace speedflow {

/**
 * Configuration for speed calculation
 * Ported from: IoT_Graduate/speedflow/settings.py
 */
struct SpeedConfig {
    float video_fps = 25.0f;
    float speed_limit_kmh = 60.0f;
    
    // Validation thresholds
    int min_track_age_frames = 12;      // ~0.5s at 25fps
    float min_world_displ_m = 0.5f;     // Minimum displacement in meters
    float max_abs_kmh = 160.0f;         // Maximum physically possible speed
    float bbox_area_jump = 2.5f;        // Max bbox area ratio change
    float min_det_conf = 0.45f;         // Minimum detection confidence
    int median_window = 5;              // Median filter window size
};

/**
 * Speed measurement data for a single track
 */
struct SpeedMeasurement {
    int track_id;
    float speed_kmh;
    int frame_number;
    bool is_valid;
    bool is_overspeeding;
};

/**
 * SpeedCalculator - Core speed calculation logic
 * Ported from: IoT_Graduate/speedflow/probes.py (SpeedProbe class)
 */
class SpeedCalculator {
public:
    explicit SpeedCalculator(std::shared_ptr<ViewTransformer> transformer,
                            const SpeedConfig& config = SpeedConfig());
    
    /**
     * Process a tracked object and calculate speed
     * @param track_id Object tracking ID
     * @param cx Center X coordinate in image
     * @param bottom_y Bottom Y coordinate in image
     * @param bbox_area Bounding box area
     * @param det_conf Detection confidence
     * @param frame_number Current frame number
     * @return Speed measurement (may be invalid if validation fails)
     */
    SpeedMeasurement processObject(int track_id,
                                   float cx,
                                   float bottom_y,
                                   float bbox_area,
                                   float det_conf,
                                   int frame_number);
    
    /**
     * Get last computed speed text for display
     * @param track_id Tracking ID
     * @return Speed text (e.g., "45 km/h")
     */
    std::string getSpeedText(int track_id) const;
    
    /**
     * Clear history for a specific track (when track is lost)
     * @param track_id Tracking ID
     */
    void clearTrack(int track_id);

private:
    std::shared_ptr<ViewTransformer> transformer_;
    SpeedConfig config_;
    
    // Track history: track_id -> deque of y_world positions
    std::unordered_map<int, std::deque<float>> history_positions_;
    
    // Speed history for median filtering
    std::unordered_map<int, std::deque<float>> speed_history_;
    
    // Track metadata
    std::unordered_map<int, int> track_birth_frame_;
    std::unordered_map<int, float> last_bbox_area_;
    std::unordered_map<int, std::string> last_speed_text_;
    std::unordered_map<int, int> last_update_frame_;
    
    /**
     * Compute speed from position history
     * @param history Deque of y_world positions
     * @return Speed in km/h (or -1 if insufficient data)
     */
    float computeSpeedKmh(const std::deque<float>& history) const;
    
    /**
     * Validate speed measurement
     * @param track_id Tracking ID
     * @param frame_no Current frame number
     * @param history Position history
     * @param speed_kmh Computed speed
     * @param area_start Initial bbox area
     * @param area_end Current bbox area
     * @param det_conf Detection confidence
     * @return true if measurement is valid
     */
    bool isValidMeasurement(int track_id,
                           int frame_no,
                           const std::deque<float>& history,
                           float speed_kmh,
                           float area_start,
                           float area_end,
                           float det_conf) const;
    
    /**
     * Apply median filter to speed
     * @param track_id Tracking ID
     * @param raw_speed Raw speed value
     * @return Filtered speed
     */
    float applyMedianFilter(int track_id, float raw_speed);
    
    /**
     * Compute median of a deque
     * @param values Input values
     * @return Median value
     */
    float computeMedian(std::deque<float> values) const;
};

} // namespace speedflow
