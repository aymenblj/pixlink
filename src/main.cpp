#include "pipeline/pipeline.hpp"
#include "pipeline/strategy_lru.hpp"
#include "pipeline/opencv_specializations.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>

#ifdef HAVE_OPENCV_CORE

/**
 * @brief Gaussian blur filter using OpenCV.
 * Applies Gaussian blur with kernel size 11x11 and sigma 3.
 */
auto gaussianBlur = [](const cv::Mat& img) {
    cv::Mat out;
    cv::GaussianBlur(img, out, cv::Size(11, 11), 3);
    return out;
};

/**
 * @brief Median blur filter using OpenCV.
 * Applies median blur with kernel size 5.
 */
auto medianBlur = [](const cv::Mat& img) {
    cv::Mat out;
    cv::medianBlur(img, out, 5);
    return out;
};

/**
 * @brief Bilateral filter using OpenCV.
 * Applies bilateral filter with diameter 9, sigmaColor 30, sigmaSpace 30.
 */
auto bilateralFilter = [](const cv::Mat& img) {
    cv::Mat out;
    cv::bilateralFilter(img, out, 9, 30, 30);
    return out;
};

/**
 * @brief Laplacian edge detection combined with the original image.
 * Converts to grayscale, applies Laplacian, inverts edges, converts back to BGR,
 * then blends with original image (70% original, 30% edges).
 */
auto laplacianEdge = [](const cv::Mat& img) {
    cv::Mat gray, lap, absLap, colorEdge, out;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::Laplacian(gray, lap, CV_16S, 3, 1, 0);
    cv::convertScaleAbs(lap, absLap, 1, 0);
    cv::bitwise_not(absLap, absLap);
    cv::cvtColor(absLap, colorEdge, cv::COLOR_GRAY2BGR);
    cv::addWeighted(img, 0.7, colorEdge, 0.3, 0, out);
    return out;
};

using Pipeline = pipeline::Pipeline<cv::Mat>;
using LRUCacheManager = pipeline::LRUCacheManager<cv::Mat>;
using CacheManager = pipeline::CacheManager<cv::Mat>;

/**
 * @brief Main program entry point.
 * Loads images from a directory, applies multiple filters, saves results with organized folder structure,
 * and manages image cache using an LRU strategy.
 */
int main() {
    try {
        const std::string inputPath = "../images/";
        const std::string outputPath = "output_images/";
        std::vector<std::string> extensions = {".jpg", ".jpeg"};

        // Create a cache manager with capacity for 100 images
        std::unique_ptr<CacheManager> cache = std::make_unique<LRUCacheManager>(100);

        // Initialize pipeline with input/output paths and cache manager
        Pipeline pipeline(inputPath, outputPath, std::move(cache));

        // Load images from the "animals" subdirectory with specified extensions
        pipeline.loadDirectory("animals", extensions);

        // Process each loaded image key by applying filters and saving outputs
        for (const auto& key : pipeline.getAllImageKeys("animals")) {
            pipeline.process(key, gaussianBlur)
                    .saveAs(key, "animals/gaussian")
                    .reset(key)
                    .process(key, medianBlur)
                    .saveAs(key, "animals/median")
                    .reset(key)
                    .process(key, bilateralFilter)
                    .saveAs(key, "animals/bilateral")
                    .reset(key)
                    .process(key, laplacianEdge)
                    .process(key, medianBlur)
                    .saveAs(key, "animals/median_laplacian", "_med_lap")
                    .release(key);
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
