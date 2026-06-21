#include "compress.h"

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <stdexcept>

namespace {

cv::Mat makeGrayImage(int size, uchar value)
{
    return cv::Mat(size, size, CV_8U, cv::Scalar(value));
}

CompressionConfig losslessConfig()
{
    CompressionConfig config;
    config.wavelet = HAAR;
    config.level = 1;
    config.Q = 0.0;
    config.threshold = 0.0;
    config.softThresh = false;
    config.colorMode = GRAY;
    return config;
}

}  // namespace

TEST(CompressImageTest, EmptyImageThrows)
{
    CompressionConfig config = losslessConfig();
    EXPECT_THROW(compressImage(cv::Mat(), config), std::runtime_error);
}

TEST(CompressImageTest, TooSmallImageThrows)
{
    CompressionConfig config = losslessConfig();
    config.level = 4;

    cv::Mat image = makeGrayImage(8, 100);
    EXPECT_THROW(compressImage(image, config), std::runtime_error);
}

TEST(CompressImageTest, GrayPipelineProducesMetrics)
{
    CompressionConfig config = losslessConfig();
    cv::Mat image = makeGrayImage(32, 120);

    CompressionResult result = compressImage(image, config);

    EXPECT_EQ(result.channels.size(), 1u);
    EXPECT_GT(result.psnr, 35.0);
    EXPECT_GT(result.compressedBytes, 0u);
    EXPECT_GT(result.compressionRatio, 0.0);
    EXPECT_EQ(result.reconstructed.rows, image.rows);
    EXPECT_EQ(result.reconstructed.cols, image.cols);
}

TEST(CompressImageTest, RgbModeProcessesThreeChannels)
{
    CompressionConfig config = losslessConfig();
    config.colorMode = RGB;
    config.Q = 10.0;
    config.threshold = 5.0;

    cv::Mat image(32, 32, CV_8UC3, cv::Scalar(30, 60, 90));
    CompressionResult result = compressImage(image, config);

    EXPECT_EQ(result.channels.size(), 3u);
    EXPECT_GT(result.compressedBytes, 0u);
    EXPECT_EQ(result.reconstructed.channels(), 3);
}

TEST(CompressImageTest, YCbCrModeKeepsDefaultProfiles)
{
    CompressionConfig config = losslessConfig();
    config.colorMode = YCBCR;
    config.Q = 10.0;
    config.threshold = 5.0;

    cv::Mat image(32, 32, CV_8UC3, cv::Scalar(40, 80, 160));
    CompressionResult result = compressImage(image, config);

    EXPECT_EQ(result.channels.size(), 3u);
    EXPECT_GT(result.psnr, 0.0);
    EXPECT_GT(result.compressedBytes, result.channels[0].compressedBytes);
}

TEST(CompressImageTest, SoftThresholdCanBeEnabled)
{
    CompressionConfig config = losslessConfig();
    config.Q = 10.0;
    config.threshold = 5.0;
    config.softThresh = true;

    cv::Mat image = makeGrayImage(32, 100);
    CompressionResult result = compressImage(image, config);

    EXPECT_GT(result.channels.front().zeroPct, 0.0);
}
