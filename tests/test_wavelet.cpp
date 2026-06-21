#include "wavelet.h"

#include <cmath>
#include <gtest/gtest.h>
#include <stdexcept>
#include <vector>

namespace {

bool vectorsNear(const std::vector<double>& a,
                 const std::vector<double>& b,
                 double tol)
{
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); i++) {
        if (std::abs(a[i] - b[i]) > tol) {
            return false;
        }
    }
    return true;
}

cv::Mat makePatternMat(int rows, int cols)
{
    cv::Mat img(rows, cols, CV_64F);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            img.at<double>(i, j) = static_cast<double>((i + 1) * 10 + j);
        }
    }
    return img;
}

}  // namespace

TEST(WaveletFilterTest, HaarFilterLengthIsTwo)
{
    WaveletFilter wf = getWavelet(HAAR);
    EXPECT_EQ(wf.Lo_D.size(), 2u);
    EXPECT_EQ(wf.Hi_D.size(), 2u);
    EXPECT_EQ(wf.Lo_R.size(), 2u);
    EXPECT_EQ(wf.Hi_R.size(), 2u);
}

TEST(WaveletFilterTest, Db4FilterLengthIsEight)
{
    WaveletFilter wf = getWavelet(DB4);
    EXPECT_EQ(wf.Lo_D.size(), 8u);
    EXPECT_EQ(wf.Hi_D.size(), 8u);
}

TEST(ParseWaveletTest, AcceptsKnownNames)
{
    EXPECT_EQ(parseWavelet("HAAR"), HAAR);
    EXPECT_EQ(parseWavelet("sym4"), SYM4);
    EXPECT_EQ(parseWavelet("db2"), DB2);
}

TEST(ParseWaveletTest, RejectsUnknownName)
{
    EXPECT_THROW(parseWavelet("INVALID"), std::invalid_argument);
}

TEST(Dwt1DTest, HaarRoundTripPreservesSignal)
{
    WaveletFilter wf = getWavelet(HAAR);
    std::vector<double> original = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<double> data = original;

    dwt1D(data, wf.Lo_D, wf.Hi_D);
    idwt1D(data, wf.Lo_R, wf.Hi_R);

    EXPECT_TRUE(vectorsNear(original, data, 1e-9));
}

TEST(Dwt2DTest, HaarRoundTripPreservesImage)
{
    WaveletFilter wf = getWavelet(HAAR);
    cv::Mat original = makePatternMat(16, 16);
    cv::Mat coeff = original.clone();

    dwt2D(coeff, wf, 1);
    idwt2D(coeff, wf, 1);

    for (int i = 0; i < original.rows; i++) {
        for (int j = 0; j < original.cols; j++) {
            EXPECT_NEAR(original.at<double>(i, j), coeff.at<double>(i, j), 1e-6);
        }
    }
}

TEST(QuantizationTest, QuantizeWithZeroStepIsNoOp)
{
    cv::Mat coeff = makePatternMat(4, 4);
    cv::Mat before = coeff.clone();

    quantize(coeff, 0.0);

    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            EXPECT_DOUBLE_EQ(before.at<double>(i, j), coeff.at<double>(i, j));
        }
    }
}

TEST(QuantizationTest, QuantizeReducesDistinctValues)
{
    cv::Mat coeff = makePatternMat(4, 4);
    quantize(coeff, 10.0);

    bool foundRounded = false;
    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            double val = coeff.at<double>(i, j);
            EXPECT_NEAR(val, std::round(val / 10.0) * 10.0, 1e-9);
            if (std::fmod(val, 10.0) == 0.0) {
                foundRounded = true;
            }
        }
    }
    EXPECT_TRUE(foundRounded);
}

TEST(ThresholdTest, HardThresholdZerosSmallCoefficients)
{
    cv::Mat coeff(2, 2, CV_64F);
    coeff.at<double>(0, 0) = 2.0;
    coeff.at<double>(0, 1) = 10.0;
    coeff.at<double>(1, 0) = -3.0;
    coeff.at<double>(1, 1) = 8.0;

    hardThreshold(coeff, 5.0);

    EXPECT_DOUBLE_EQ(coeff.at<double>(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(coeff.at<double>(0, 1), 10.0);
    EXPECT_DOUBLE_EQ(coeff.at<double>(1, 0), 0.0);
    EXPECT_DOUBLE_EQ(coeff.at<double>(1, 1), 8.0);
}

TEST(ThresholdTest, SoftThresholdShrinksLargeCoefficients)
{
    cv::Mat coeff(1, 2, CV_64F);
    coeff.at<double>(0, 0) = 8.0;
    coeff.at<double>(0, 1) = -2.0;

    softThreshold(coeff, 3.0);

    EXPECT_DOUBLE_EQ(coeff.at<double>(0, 0), 5.0);
    EXPECT_DOUBLE_EQ(coeff.at<double>(0, 1), 0.0);
}

TEST(ZeroRatioTest, CountsZeroEntries)
{
    cv::Mat coeff(2, 2, CV_64F, cv::Scalar(0));
    coeff.at<double>(0, 0) = 5.0;

    EXPECT_NEAR(zeroRatio(coeff), 0.75, 1e-9);
}

TEST(IsValidSizeTest, RejectsTooSmallImages)
{
    EXPECT_FALSE(isValidSize(8, 8, 4));
    EXPECT_TRUE(isValidSize(16, 16, 4));
    EXPECT_TRUE(isValidSize(32, 64, 2));
}
