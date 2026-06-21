#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

enum WaveletType { HAAR, DB2, DB4, SYM4 };

struct WaveletFilter {
    std::vector<double> Lo_D, Hi_D, Lo_R, Hi_R;
};

WaveletFilter getWavelet(WaveletType type);
WaveletType parseWavelet(const std::string& name);

void dwt1D(std::vector<double>& data,
           const std::vector<double>& Lo,
           const std::vector<double>& Hi);

void idwt1D(std::vector<double>& data,
            const std::vector<double>& Lo,
            const std::vector<double>& Hi);

void dwt2D(cv::Mat& img, const WaveletFilter& wf, int levels);
void idwt2D(cv::Mat& img, const WaveletFilter& wf, int levels);

void quantize(cv::Mat& coeff, double Q);
void hardThreshold(cv::Mat& coeff, double T);
void softThreshold(cv::Mat& coeff, double T);
double zeroRatio(const cv::Mat& coeff);

bool isValidSize(int rows, int cols, int level);

cv::Mat showLevel(const cv::Mat& coeff, int level);
