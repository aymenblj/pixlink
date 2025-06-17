#pragma once
#include <vector>

/**
 * @brief Metadata for per-image region data.
 * This file is optional, but convenient if you want to track region (box) metadata alongside your images.
 */
template <typename ImageType, typename RectType>
struct ImageRegionMeta {
    ImageType image;
    std::vector<RectType> regions;
    bool regionsDetected = false;
};