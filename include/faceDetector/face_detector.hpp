#pragma once

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <vector>
#include <functional>

/**
 * @brief FaceDetector for DNN face detection.
 * - .operator() returns annotated image with boxes.
 * - .countFaces returns number of faces.
 * - .detect returns vector of face boxes, with optional filter.
 */
class FaceDetector {
public:
    FaceDetector(const std::string& protoPath, const std::string& modelPath)
        : net(cv::dnn::readNetFromCaffe(protoPath, modelPath)) {}

    cv::Mat operator()(const cv::Mat& img) const {
        cv::Mat blob = cv::dnn::blobFromImage(img, 1.0, cv::Size(300, 300), cv::Scalar(104.0, 177.0, 123.0));
        net.setInput(blob);
        cv::Mat detections = net.forward();

        cv::Mat out = img.clone();
        float* data = (float*)detections.data;
        const int numDetections = detections.size[2];

        for (int i = 0; i < numDetections; ++i) {
            float confidence = data[i * 7 + 2];
            if (confidence > 0.5) {
                int x1 = static_cast<int>(data[i * 7 + 3] * img.cols);
                int y1 = static_cast<int>(data[i * 7 + 4] * img.rows);
                int x2 = static_cast<int>(data[i * 7 + 5] * img.cols);
                int y2 = static_cast<int>(data[i * 7 + 6] * img.rows);
                cv::rectangle(out, cv::Rect(x1, y1, x2 - x1, y2 - y1), cv::Scalar(0, 255, 0), 2);
            }
        }
        return out;
    }

    // Returns number of faces with confidence > 0.5
    int countFaces(const cv::Mat& img) const {
        return static_cast<int>(detect(img).size());
    }

    // Returns face rectangles with optional lambda filter: (Rect, confidence) -> bool
    std::vector<cv::Rect> detect(const cv::Mat& img,
        std::function<bool(const cv::Rect&, float)> filter = nullptr) const
    {
        cv::Mat blob = cv::dnn::blobFromImage(img, 1.0, cv::Size(300, 300), cv::Scalar(104, 177, 123));
        net.setInput(blob);
        cv::Mat detections = net.forward();

        std::vector<cv::Rect> boxes;
        float* data = (float*)detections.data;
        int numDetections = detections.size[2];
        for (int i = 0; i < numDetections; ++i) {
            float confidence = data[i * 7 + 2];
            int x1 = static_cast<int>(data[i * 7 + 3] * img.cols);
            int y1 = static_cast<int>(data[i * 7 + 4] * img.rows);
            int x2 = static_cast<int>(data[i * 7 + 5] * img.cols);
            int y2 = static_cast<int>(data[i * 7 + 6] * img.rows);
            cv::Rect box(x1, y1, x2 - x1, y2 - y1);
            if (confidence > 0.5 && (!filter || filter(box, confidence))) {
                boxes.push_back(box);
            }
        }
        return boxes;
    }

private:
    mutable cv::dnn::Net net;
};