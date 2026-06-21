#pragma once

#include <opencv2/opencv.hpp>
#include <cstddef>

double channelPSNR(const cv::Mat& original, const cv::Mat& reconstructed);
double colorPSNR(const cv::Mat& original, const cv::Mat& reconstructed);
double compressionRatio(size_t originalBytes, size_t compressedBytes);
