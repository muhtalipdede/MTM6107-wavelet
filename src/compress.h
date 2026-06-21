#pragma once

#include "color.h"
#include "wavelet.h"

#include <opencv2/opencv.hpp>
#include <cstddef>
#include <string>
#include <vector>

struct CompressionConfig {
    WaveletType wavelet = SYM4;
    int level = 2;
    double Q = 10.0;
    double threshold = 5.0;
    bool softThresh = false;
    ColorMode colorMode = GRAY;
};

struct ChannelResult {
    cv::Mat reconstructed;
    std::vector<uint8_t> encoded;
    double psnr = 0.0;
    double zeroPct = 0.0;
    size_t compressedBytes = 0;
};

struct CompressionResult {
    cv::Mat original;
    cv::Mat reconstructed;
    std::vector<ChannelResult> channels;
    double psnr = 0.0;
    double compressionRatio = 0.0;
    size_t originalBytes = 0;
    size_t compressedBytes = 0;
    CompressionConfig config;
};

CompressionResult compressImage(const cv::Mat& image, const CompressionConfig& config);
void printResult(const CompressionResult& result);
