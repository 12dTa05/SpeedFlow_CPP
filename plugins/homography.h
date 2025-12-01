#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

namespace speedflow {

/**
 * ViewTransformer - Perspective transformation for image to world coordinates
 * Ported from: IoT_Graduate/speedflow/homography.py
 */
class ViewTransformer {
public:
    /**
     * Constructor
     * @param source Source points in image coordinates (4 points)
     * @param target Target points in world coordinates (4 points)
     */
    ViewTransformer(const std::vector<cv::Point2f>& source,
                    const std::vector<cv::Point2f>& target);
    
    /**
     * Transform points from image to world coordinates
     * @param points Input points in image coordinates
     * @return Transformed points in world coordinates
     */
    std::vector<cv::Point2f> transformPoints(const std::vector<cv::Point2f>& points) const;
    
    /**
     * Transform a single point
     * @param point Input point in image coordinates
     * @return Transformed point in world coordinates
     */
    cv::Point2f transformPoint(const cv::Point2f& point) const;

private:
    cv::Mat homography_matrix_;
};

} // namespace speedflow
