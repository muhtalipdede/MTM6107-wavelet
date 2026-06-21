#include "color.h"

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <stdexcept>

TEST(ParseColorModeTest, AcceptsKnownModes)
{
    EXPECT_EQ(parseColorMode("gray"), GRAY);
    EXPECT_EQ(parseColorMode("RGB"), RGB);
    EXPECT_EQ(parseColorMode("ycbcr"), YCBCR);
}

TEST(ParseColorModeTest, RejectsUnknownMode)
{
    EXPECT_THROW(parseColorMode("hsv"), std::invalid_argument);
}

TEST(GetProfilesTest, ReturnsExpectedChannelCounts)
{
    EXPECT_EQ(getProfiles(GRAY).size(), 1u);
    EXPECT_EQ(getProfiles(RGB).size(), 3u);
    EXPECT_EQ(getProfiles(YCBCR).size(), 3u);
}

TEST(GetProfilesTest, YCbCrUsesDifferentProfilesForYAndChrominance)
{
    std::vector<ChannelProfile> profiles = getProfiles(YCBCR);

    EXPECT_DOUBLE_EQ(profiles[0].quantStep, 1.0);
    EXPECT_DOUBLE_EQ(profiles[0].threshold, 0.0);
    EXPECT_DOUBLE_EQ(profiles[1].quantStep, 8.0);
    EXPECT_DOUBLE_EQ(profiles[2].threshold, 5.0);
}

TEST(ColorConversionTest, YCbCrRoundTripPreservesBGR)
{
    cv::Mat bgr(4, 4, CV_8UC3, cv::Scalar(120, 80, 200));

    cv::Mat y, cb, cr;
    rgbToYCbCr(bgr, y, cb, cr);
    cv::Mat restored = ycbcrToBGR(y, cb, cr);

    for (int i = 0; i < bgr.rows; i++) {
        for (int j = 0; j < bgr.cols; j++) {
            cv::Vec3b expected = bgr.at<cv::Vec3b>(i, j);
            cv::Vec3b actual = restored.at<cv::Vec3b>(i, j);
            EXPECT_NEAR(actual[0], expected[0], 1);
            EXPECT_NEAR(actual[1], expected[1], 1);
            EXPECT_NEAR(actual[2], expected[2], 1);
        }
    }
}

TEST(SplitMergeTest, GrayModeReturnsSingleChannel)
{
    cv::Mat gray(8, 8, CV_8U, cv::Scalar(50));
    std::vector<cv::Mat> channels = splitChannels(gray, GRAY);

    ASSERT_EQ(channels.size(), 1u);
    cv::Mat merged = mergeChannels(channels, GRAY, gray.size());
    EXPECT_EQ(merged.rows, gray.rows);
    EXPECT_EQ(merged.cols, gray.cols);
}

TEST(SplitMergeTest, RgbModeSplitsAndMergesThreeChannels)
{
    cv::Mat bgr(8, 8, CV_8UC3, cv::Scalar(10, 20, 30));
    std::vector<cv::Mat> channels = splitChannels(bgr, RGB);

    ASSERT_EQ(channels.size(), 3u);
    cv::Mat merged = mergeChannels(channels, RGB, bgr.size());
    cv::Mat diff;
    cv::absdiff(merged, bgr, diff);
    EXPECT_EQ(cv::countNonZero(diff.reshape(1, diff.total())), 0);
}
