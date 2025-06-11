#pragma once

#ifdef HAVE_OPENCV_CORE

#include "pipeline/strategy_default.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>
#include <filesystem>
#include <stdexcept>

namespace pipeline {

/**
 * @brief Specialization of DefaultImageLoader for cv::Mat.
 * 
 * Loads an image from a file using OpenCV's imread.
 * Throws std::runtime_error if loading fails.
 */
template <>
inline cv::Mat DefaultImageLoader<cv::Mat>::loadFromFile(const std::string& path) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) throw std::runtime_error("Failed to load: " + path);
    return img;
}

/**
 * @brief Specialization of DefaultImageSaver for cv::Mat.
 * 
 * Saves an image to a file using OpenCV's imwrite.
 * Creates the necessary directories if they do not exist.
 * 
 * @param outputPath Path to save the image.
 * @param image The cv::Mat image to save.
 */
template <>
inline void DefaultImageSaver<cv::Mat>::save(const std::string& outputPath, const cv::Mat& image) {
    std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());
    cv::imwrite(outputPath, image);
}

} // namespace pipeline

#endif // HAVE_OPENCV_CORE
