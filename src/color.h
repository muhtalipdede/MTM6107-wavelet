#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

enum ColorMode { GRAY, RGB, YCBCR };

ColorMode parseColorMode(const std::string& name);

struct ChannelProfile {
    double quantStep;
    double threshold;
};

void rgbToYCbCr(const cv::Mat& bgr, cv::Mat& y, cv::Mat& cb, cv::Mat& cr);
cv::Mat ycbcrToBGR(const cv::Mat& y, const cv::Mat& cb, const cv::Mat& cr);

std::vector<cv::Mat> splitChannels(const cv::Mat& image, ColorMode mode);
cv::Mat mergeChannels(const std::vector<cv::Mat>& channels, ColorMode mode, const cv::Size& size);
std::vector<ChannelProfile> getProfiles(ColorMode mode);
