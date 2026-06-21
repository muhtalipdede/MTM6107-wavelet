#include "metrics.h"

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

TEST(MetricsTest, ChannelPSNRIsPerfectForIdenticalImages)
{
    cv::Mat img(8, 8, CV_8U, cv::Scalar(128));
    EXPECT_DOUBLE_EQ(channelPSNR(img, img), 100.0);
}

TEST(MetricsTest, ChannelPSNRDecreasesWithNoise)
{
    cv::Mat original(16, 16, CV_8U, cv::Scalar(100));
    cv::Mat noisy = original.clone();
    noisy.at<uchar>(0, 0) = 200;

    EXPECT_GT(channelPSNR(original, original), channelPSNR(original, noisy));
}

TEST(MetricsTest, CompressionRatioDividesOriginalByCompressed)
{
    EXPECT_DOUBLE_EQ(compressionRatio(1000, 250), 4.0);
    EXPECT_DOUBLE_EQ(compressionRatio(512, 0), 0.0);
}

TEST(MetricsTest, ColorPSNRHandlesIdenticalBGRImages)
{
    cv::Mat img(8, 8, CV_8UC3, cv::Scalar(10, 20, 30));
    EXPECT_DOUBLE_EQ(colorPSNR(img, img), 100.0);
}
