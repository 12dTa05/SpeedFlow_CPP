#include "speed_calculator.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace speedflow {

SpeedCalculator::SpeedCalculator(std::shared_ptr<ViewTransformer> transformer,
                                 const SpeedConfig& config)
    : transformer_(transformer), config_(config) {
}

SpeedMeasurement SpeedCalculator::processObject(int track_id,
                                               float cx,
                                               float bottom_y,
                                               float bbox_area,
                                               float det_conf,
                                               int frame_number) {
    SpeedMeasurement result;
    result.track_id = track_id;
    result.frame_number = frame_number;
    result.is_valid = false;
    result.is_overspeeding = false;
    result.speed_kmh = 0.0f;
    
    // Track birth frame
    if (track_birth_frame_.find(track_id) == track_birth_frame_.end()) {
        track_birth_frame_[track_id] = frame_number;
    }
    
    // Transform point to world coordinates
    cv::Point2f image_point(cx, bottom_y);
    cv::Point2f world_point = transformer_->transformPoint(image_point);
    float y_world = world_point.y;
    
    // Add to history
    auto& history = history_positions_[track_id];
    history.push_back(y_world);
    
    // Need full window for speed calculation
    if (history.size() < static_cast<size_t>(config_.video_fps)) {
        return result;
    }
    
    // Compute raw speed
    float raw_speed = computeSpeedKmh(history);
    if (raw_speed < 0) {
        return result;
    }
    
    // Get bbox area history
    float area_start = last_bbox_area_.count(track_id) ? last_bbox_area_[track_id] : bbox_area;
    last_bbox_area_[track_id] = bbox_area;
    
    // Validate measurement
    if (!isValidMeasurement(track_id, frame_number, history, raw_speed,
                           area_start, bbox_area, det_conf)) {
        return result;
    }
    
    // Apply median filter
    float filtered_speed = applyMedianFilter(track_id, raw_speed);
    
    // Update result
    result.speed_kmh = filtered_speed;
    result.is_valid = true;
    result.is_overspeeding = (filtered_speed > config_.speed_limit_kmh);
    
    // Update display text
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << filtered_speed << " km/h";
    last_speed_text_[track_id] = oss.str();
    last_update_frame_[track_id] = frame_number;
    
    return result;
}

std::string SpeedCalculator::getSpeedText(int track_id) const {
    auto it = last_speed_text_.find(track_id);
    if (it != last_speed_text_.end()) {
        return it->second;
    }
    return "";
}

void SpeedCalculator::clearTrack(int track_id) {
    history_positions_.erase(track_id);
    speed_history_.erase(track_id);
    track_birth_frame_.erase(track_id);
    last_bbox_area_.erase(track_id);
    last_speed_text_.erase(track_id);
    last_update_frame_.erase(track_id);
}

float SpeedCalculator::computeSpeedKmh(const std::deque<float>& history) const {
    if (history.size() < static_cast<size_t>(config_.video_fps)) {
        return -1.0f;
    }
    
    float distance_m = std::abs(history.back() - history.front());
    float time_s = (history.size() - 1) / config_.video_fps;
    
    if (time_s <= 0) {
        return 0.0f;
    }
    
    // Convert m/s to km/h
    return (distance_m / time_s) * 3.6f;
}

bool SpeedCalculator::isValidMeasurement(int track_id,
                                         int frame_no,
                                         const std::deque<float>& history,
                                         float speed_kmh,
                                         float area_start,
                                         float area_end,
                                         float det_conf) const {
    // 1. Track age validation
    int birth_frame = track_birth_frame_.at(track_id);
    int age_frames = frame_no - birth_frame;
    if (age_frames < config_.min_track_age_frames) {
        return false;
    }
    
    // 2. Minimum displacement validation
    if (history.size() >= 2) {
        float displacement_m = std::abs(history.back() - history.front());
        if (displacement_m < config_.min_world_displ_m) {
            return false;
        }
    }
    
    // 3. Physical speed limit validation
    if (speed_kmh <= 0 || speed_kmh > config_.max_abs_kmh) {
        return false;
    }
    
    // 4. Bbox stability validation
    if (area_start > 0 && area_end / area_start > config_.bbox_area_jump) {
        return false;
    }
    
    // 5. Detection confidence validation
    if (det_conf < config_.min_det_conf) {
        return false;
    }
    
    return true;
}

float SpeedCalculator::applyMedianFilter(int track_id, float raw_speed) {
    auto& history = speed_history_[track_id];
    history.push_back(raw_speed);
    
    // Keep only last N values
    while (history.size() > static_cast<size_t>(config_.median_window)) {
        history.pop_front();
    }
    
    return computeMedian(history);
}

float SpeedCalculator::computeMedian(std::deque<float> values) const {
    if (values.empty()) {
        return 0.0f;
    }
    
    std::vector<float> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0f;
    } else {
        return sorted[n/2];
    }
}

} // namespace speedflow
