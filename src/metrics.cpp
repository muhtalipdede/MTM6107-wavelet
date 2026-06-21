#include "metrics.h"

#include <cmath>

// Tek kanal icin PSNR hesaplar.
//   MSE = (1/N) * sum_i (O_i - R_i)^2
//   PSNR = 10 * log10(255^2 / MSE)  [dB]
double channelPSNR(const cv::Mat& original, const cv::Mat& reconstructed)
{
    cv::Mat diff;
    cv::absdiff(original, reconstructed, diff);
    diff.convertTo(diff, CV_32F);
    diff = diff.mul(diff);

    double mse = cv::sum(diff)[0] / original.total();
    if (mse == 0.0) {
        return 100.0;
    }

    return 10.0 * std::log10((255.0 * 255.0) / mse);
}

// Renkli goruntu icin 3 kanalin ortalama MSE'sine gore PSNR hesaplar.
//   MSE_c = (1/N) * sum_i (O_c(i) - R_c(i))^2,  c in {B,G,R}
//   MSE_avg = (MSE_B + MSE_G + MSE_R) / 3
//   PSNR = 10 * log10(255^2 / MSE_avg)  [dB]
double colorPSNR(const cv::Mat& original, const cv::Mat& reconstructed)
{
    cv::Mat diff;
    cv::absdiff(original, reconstructed, diff);
    diff.convertTo(diff, CV_32F);
    diff = diff.mul(diff);

    cv::Scalar mse = cv::sum(diff);
    double mseVal = (mse[0] + mse[1] + mse[2]) / (3.0 * original.total());
    if (mseVal == 0.0) {
        return 100.0;
    }

    return 10.0 * std::log10((255.0 * 255.0) / mseVal);
}

// Sikistirma oranini hesaplar.
//   CR = B_original / B_compressed
double compressionRatio(size_t originalBytes, size_t compressedBytes)
{
    if (compressedBytes == 0) {
        return 0.0;
    }
    return static_cast<double>(originalBytes) / static_cast<double>(compressedBytes);
}
