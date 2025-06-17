#pragma once
#include <vector>
#include <functional>

/**
 * @brief Applies a filter to specified rectangular regions of an image.
 * The filter must be a function/lambda: ImageType(const ImageType&)
 * The function is generic and works for any ImageType supporting ROI, clone, and copyTo.
 */
template <typename ImageType, typename RectType>
ImageType applyFilterToRegions(const ImageType& img, const std::vector<RectType>& boxes, const std::function<ImageType(const ImageType&)>& filter) {
    ImageType result = img.clone();
    for (const auto& rect : boxes) {
        // Crop ROI safely
        RectType roi = rect & RectType(0, 0, result.cols, result.rows);
        if (roi.width > 0 && roi.height > 0) {
            ImageType patch = result(roi);
            ImageType filtered = filter(patch);
            filtered.copyTo(result(roi));
        }
    }
    return result;
}