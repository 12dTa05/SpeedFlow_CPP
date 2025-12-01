#include "homography.h"
#include <stdexcept>

namespace speedflow {

ViewTransformer::ViewTransformer(const std::vector<cv::Point2f>& source,
                                 const std::vector<cv::Point2f>& target) {
    if (source.size() != 4 || target.size() != 4) {
        throw std::invalid_argument("ViewTransformer requires exactly 4 source and 4 target points");
    }
    
    // Compute perspective transformation matrix
    homography_matrix_ = cv::getPerspectiveTransform(source, target);
}

std::vector<cv::Point2f> ViewTransformer::transformPoints(
    const std::vector<cv::Point2f>& points) const {
    
    if (points.empty()) {
        return {};
    }
    
    std::vector<cv::Point2f> transformed;
    cv::perspectiveTransform(points, transformed, homography_matrix_);
    
    return transformed;
}

cv::Point2f ViewTransformer::transformPoint(const cv::Point2f& point) const {
    std::vector<cv::Point2f> input = {point};
    auto output = transformPoints(input);
    return output[0];
}

} // namespace speedflow
