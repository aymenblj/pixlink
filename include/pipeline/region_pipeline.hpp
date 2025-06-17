#pragma once

#include "pipeline/region_filter.hpp"
#include "pipeline/faces_meta.hpp"
#include <unordered_map>
#include <string>
#include <functional>

namespace pipeline {

template <typename ImageType, typename RectType = cv::Rect>
class RegionPipeline {
public:
    using DetectorFunc = std::function<std::vector<RectType>(const ImageType&)>;
    using MetaMap = std::unordered_map<std::string, ImageRegionMeta<ImageType, RectType>>;
    using ImageMap = std::unordered_map<std::string, ImageType>;

    RegionPipeline(DetectorFunc detector, ImageMap& workingMap)
        : detector(detector), workingMap(workingMap) {}

    MetaMap metaMap;

    // In-place region processing!
    RegionPipeline& processRegion(const std::string& key, const std::function<void(ImageType&, const RectType&)>& filter) {
        auto it = workingMap.find(key);
        if (it == workingMap.end()) throw std::runtime_error("Key not found: " + key);

        auto& img = it->second;

        auto& meta = metaMap[key];
        if (!meta.regionsDetected) {
            meta.regions = detector(img);
            meta.regionsDetected = true;
        }

        for (const auto& rect : meta.regions) {
            RectType roi = rect & RectType(0, 0, img.cols, img.rows);
            filter(img, roi);
        }

        return *this;
    }

    RegionPipeline& resetRegion(const std::string& key) {
        metaMap.erase(key);
        return *this;
    }

private:
    DetectorFunc detector;
    ImageMap& workingMap;
};

} // namespace pipeline