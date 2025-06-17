#include "pipeline/pipeline.hpp"
#include "pipeline/strategy_lru.hpp"
#include "pipeline/opencv_specializations.hpp"
#include "faceDetector/face_detector.hpp"
#include "pipeline/region_pipeline.hpp"
#include <opencv2/core/utils/logger.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

#ifdef HAVE_OPENCV_CORE

/**
 * @brief Strong Gaussian blur filter using OpenCV, in-place on ROI.
 * Parameters: Very strong blur (large kernel, high sigma)
 */
auto gaussianBlurInPlace = [](cv::Mat& img, const cv::Rect& roi) {
    // strong Gaussian blur
    cv::GaussianBlur(img(roi), img(roi), cv::Size(151, 151), 80, 80, cv::BORDER_DEFAULT);
};

/**
 * @brief Medium-strong Median blur filter using OpenCV, in-place on ROI.
 * Parameters: Still strong, but less than the above Gaussian.
 */
auto medianBlurInPlace = [](cv::Mat& img, const cv::Rect& roi) {
    // Slightly less strong than the Gaussian, still blocks details
    cv::medianBlur(img(roi), img(roi), 55); // must be odd and > 1, but noticeably less than 151
};

/**
 * @brief Moderate Pixelation (blocky mosaic) filter using OpenCV, in-place on ROI.
 * Parameters: Less blocky than previous, but still masks identity.
 */
auto pixelateInPlace = [](cv::Mat& img, const cv::Rect& roi, int blockSize = 12) {
    cv::Mat region = img(roi);

    // Compute new size (avoid zero)
    int w = std::max(1, region.cols / blockSize);
    int h = std::max(1, region.rows / blockSize);

    // Downscale then upscale for pixelation
    cv::Mat small;
    cv::resize(region, small, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);
    cv::resize(small, region, region.size(), 0, 0, cv::INTER_NEAREST);
};

using Pipeline = pipeline::Pipeline<cv::Mat>;
using LRUCacheManager = pipeline::LRUCacheManager<cv::Mat>;
using CacheManager = pipeline::CacheManager<cv::Mat>;
using RegionPipeline = pipeline::RegionPipeline<cv::Mat, cv::Rect>;

/**
 * @brief Main program entry point.
 * Loads images from a directory, applies multiple filters, saves results with organized folder structure,
 * and manages image cache using an LRU strategy.
 */
int main() {
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);
    try {
        const std::string inputPath = "../images/";
        const std::string outputPath = "output_images/";
        std::vector<std::string> extensions = {".jpg", ".jpeg"}; // Add more extensions

        // Create a cache manager with capacity for 100 images
        std::unique_ptr<CacheManager> cache = std::make_unique<LRUCacheManager>(100);

        // Initialize pipeline with input/output paths and cache manager
        Pipeline pipeline(inputPath, outputPath, std::move(cache));

        // Process each loaded image key by applying filters and saving outputs
        FaceDetector detector("../deploy.prototxt", "../res10_300x300_ssd_iter_140000_fp16.caffemodel");

        // Region pipeline for region-based processing without modifying generic pipeline
        auto detectorFunc = [&](const cv::Mat &img){ return detector.detect(img); };
        RegionPipeline regionPipeline(detectorFunc, pipeline.getWorkingMap());

        // --------- Processing ----------
        // Load images from the inputPath directory with specified extensions
        pipeline.loadDirectory("people", extensions);

        // Filter pipeline to keep only images with faces
        pipeline.filter([&](const std::string &, const cv::Mat &img)
                        { return detector.countFaces(img) > 0; });
        
        // Now process only the images with faces detected
        for (const auto &key : pipeline.getAllImageKeys()) {
            // Show face-detector-filter detections and counts
            int count = detector.countFaces(pipeline.getWorkingMap().at(key));
            std::cout << "faces detected in " << key << ": " << count << "\n";

            regionPipeline.processRegion(key, gaussianBlurInPlace);
            pipeline.saveAs(key, "people/gaussian");
            regionPipeline.resetRegion(key);
            pipeline.reset(key);

            regionPipeline.processRegion(key, medianBlurInPlace);
            pipeline.saveAs(key, "people/median");
            regionPipeline.resetRegion(key);
            pipeline.reset(key);

            regionPipeline.processRegion(key, pixelateInPlace);
            pipeline.saveAs(key, "people/pixelateInPlace");
            regionPipeline.resetRegion(key);
            pipeline.reset(key);

            // You can add more filter chains here as needed

            pipeline.unload(key); // Optionally unload
        }

        std::cout << "Processing completed successfully.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

#else

/**
 * @brief Fallback main if OpenCV is not available.
 * Prints an error message and exits with failure code.
 */
int main() {
    std::cerr << "OpenCV support is not available in this build.\n";
    return 1;
}

#endif