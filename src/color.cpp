#include "color.h"

#include <stdexcept>

// Komut satirindan gelen renk modu adini enum degerine cevirir.
ColorMode parseColorMode(const std::string& name)
{
    if (name == "gray" || name == "GRAY") return GRAY;
    if (name == "rgb" || name == "RGB") return RGB;
    if (name == "ycbcr" || name == "YCBCR") return YCBCR;
    throw std::invalid_argument("Unknown color mode: " + name);
}

// BGR goruntusunu Y, Cb, Cr kanallarina ayirir (ITU-R BT.601).
//   Y  =  0.299R + 0.587G + 0.114B
//   Cb = -0.169R - 0.331G + 0.500B + 128
//   Cr =  0.500R - 0.419G - 0.081B + 128
void rgbToYCbCr(const cv::Mat& bgr, cv::Mat& y, cv::Mat& cb, cv::Mat& cr)
{
    y = cv::Mat(bgr.rows, bgr.cols, CV_64F);
    cb = cv::Mat(bgr.rows, bgr.cols, CV_64F);
    cr = cv::Mat(bgr.rows, bgr.cols, CV_64F);

    for (int i = 0; i < bgr.rows; i++) {
        for (int j = 0; j < bgr.cols; j++) {
            cv::Vec3b pixel = bgr.at<cv::Vec3b>(i, j);
            double B = pixel[0];
            double G = pixel[1];
            double R = pixel[2];

            y.at<double>(i, j) = 0.299 * R + 0.587 * G + 0.114 * B;
            cb.at<double>(i, j) = -0.169 * R - 0.331 * G + 0.500 * B + 128.0;
            cr.at<double>(i, j) = 0.500 * R - 0.419 * G - 0.081 * B + 128.0;
        }
    }
}

// Y, Cb, Cr kanallarini tekrar BGR goruntusune donusturur.
//   R = Y + 1.402 * (Cr - 128)
//   G = Y - 0.344 * (Cb - 128) - 0.714 * (Cr - 128)
//   B = Y + 1.772 * (Cb - 128)
cv::Mat ycbcrToBGR(const cv::Mat& y, const cv::Mat& cb, const cv::Mat& cr)
{
    cv::Mat bgr(y.rows, y.cols, CV_8UC3);

    for (int i = 0; i < y.rows; i++) {
        for (int j = 0; j < y.cols; j++) {
            double Y = y.at<double>(i, j);
            double Cb = cb.at<double>(i, j) - 128.0;
            double Cr = cr.at<double>(i, j) - 128.0;

            double R = Y + 1.402 * Cr;
            double G = Y - 0.344 * Cb - 0.714 * Cr;
            double B = Y + 1.772 * Cb;

            bgr.at<cv::Vec3b>(i, j) = cv::Vec3b(
                cv::saturate_cast<uchar>(B),
                cv::saturate_cast<uchar>(G),
                cv::saturate_cast<uchar>(R));
        }
    }

    return bgr;
}

// Renk moduna gore goruntuyu islenecek kanallara ayirir (gri / BGR / YCbCr).
std::vector<cv::Mat> splitChannels(const cv::Mat& image, ColorMode mode)
{
    std::vector<cv::Mat> channels;

    if (mode == GRAY) {
        cv::Mat gray;
        if (image.channels() == 1) {
            gray = image.clone();
        } else {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        }
        channels.push_back(gray);
        return channels;
    }

    if (mode == RGB) {
        cv::split(image, channels);
        return channels;
    }

    cv::Mat y, cb, cr;
    rgbToYCbCr(image, y, cb, cr);
    channels.push_back(y);
    channels.push_back(cb);
    channels.push_back(cr);
    return channels;
}

// Sikistirilmis kanallari secilen renk moduna gore tek goruntude birlestirir.
cv::Mat mergeChannels(const std::vector<cv::Mat>& channels, ColorMode mode, const cv::Size& size)
{
    if (mode == GRAY) {
        return channels.front();
    }

    if (mode == RGB) {
        cv::Mat merged;
        cv::merge(channels, merged);
        return merged;
    }

    cv::Mat y, cb, cr;
    channels[0].convertTo(y, CV_64F);
    channels[1].convertTo(cb, CV_64F);
    channels[2].convertTo(cr, CV_64F);
    return ycbcrToBGR(y, cb, cr);
}

// Renk moduna gore kanal bazli Q ve threshold profillerini dondurur.
std::vector<ChannelProfile> getProfiles(ColorMode mode)
{
    if (mode == GRAY) {
        return {{10.0, 5.0}};
    }

    if (mode == RGB) {
        return {
            {10.0, 5.0},
            {10.0, 5.0},
            {10.0, 5.0}
        };
    }

    // YCbCr: Y kanali hafif, Cb/Cr kanallari agresif sikistirilir.
    return {
        {1.0, 0.0},
        {8.0, 5.0},
        {8.0, 5.0}
    };
}
