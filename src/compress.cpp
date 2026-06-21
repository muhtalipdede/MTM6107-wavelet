#include "compress.h"

#include "huffman.h"
#include "metrics.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {

// Kanali wavelet islemi icin CV_64F formatina cevirir.
cv::Mat toWorkChannel(const cv::Mat& channel)
{
    cv::Mat work;
    if (channel.type() == CV_64F) {
        work = channel.clone();
    } else {
        channel.convertTo(work, CV_64F);
    }
    return work;
}

// Kanali PSNR hesabi ve goruntuleme icin 8-bit gri formata cevirir.
cv::Mat toGray8U(const cv::Mat& channel)
{
    cv::Mat gray;
    if (channel.type() == CV_64F) {
        channel.convertTo(gray, CV_8U);
    } else if (channel.type() == CV_8U) {
        gray = channel;
    } else {
        channel.convertTo(gray, CV_8U);
    }
    return gray;
}

// Tek kanal uzerinde DWT -> quantize -> threshold -> Huffman -> IDWT pipeline'ini calistirir.
//   x -> DWT -> Q(w) -> T(w) -> Huffman -> IDWT -> x_hat
ChannelResult compressChannel(const cv::Mat& channel,
                              const WaveletFilter& wf,
                              const CompressionConfig& config,
                              const ChannelProfile& profile)
{
    ChannelResult result;

    cv::Mat channelD = toWorkChannel(channel);
    cv::Mat coeff = channelD.clone();
    dwt2D(coeff, wf, config.level);

    quantize(coeff, profile.quantStep);
    if (config.softThresh) {
        softThreshold(coeff, profile.threshold);
    } else {
        hardThreshold(coeff, profile.threshold);
    }

    result.zeroPct = zeroRatio(coeff) * 100.0;

    std::vector<int> data;
    data.reserve(static_cast<size_t>(coeff.rows * coeff.cols));
    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            data.push_back(static_cast<int>(std::lround(coeff.at<double>(i, j))));
        }
    }

    HuffmanNode* root = nullptr;
    std::map<int, std::string> code;
    result.encoded = huffmanEncode(data, root, code);
    result.compressedBytes = result.encoded.size();
    freeHuffmanTree(root);

    cv::Mat rec = coeff.clone();
    idwt2D(rec, wf, config.level);
    rec.convertTo(result.reconstructed, CV_8U);

    cv::Mat original8U = toGray8U(channel);
    result.psnr = channelPSNR(original8U, result.reconstructed);
    return result;
}

// Konsol ciktisi icin kanal adini dondurur (Gray, B/G/R veya Y/Cb/Cr).
std::string channelLabel(ColorMode mode, int index)
{
    if (mode == GRAY) return "Gray";
    if (mode == RGB) {
        static const char* names[] = {"B", "G", "R"};
        return names[index];
    }

    static const char* names[] = {"Y", "Cb", "Cr"};
    return names[index];
}

}  // namespace

// Goruntuyu kanal(lar)a ayirip sikistirir; PSNR ve sikistirma oranini hesaplar.
CompressionResult compressImage(const cv::Mat& image, const CompressionConfig& config)
{
    if (image.empty()) {
        throw std::runtime_error("Input image is empty.");
    }

    if (!isValidSize(image.rows, image.cols, config.level)) {
        throw std::runtime_error("Image size is too small for the selected DWT level.");
    }

    CompressionResult result;
    result.config = config;
    result.original = image.clone();

    WaveletFilter wf = getWavelet(config.wavelet);
    std::vector<cv::Mat> inputChannels = splitChannels(image, config.colorMode);
    std::vector<ChannelProfile> profiles = getProfiles(config.colorMode);

    if (config.colorMode == GRAY) {
        profiles[0].quantStep = config.Q;
        profiles[0].threshold = config.threshold;
    } else if (config.colorMode == RGB) {
        for (auto& profile : profiles) {
            profile.quantStep = config.Q;
            profile.threshold = config.threshold;
        }
    }

    std::vector<cv::Mat> reconstructedChannels;
    reconstructedChannels.reserve(inputChannels.size());

    for (size_t i = 0; i < inputChannels.size(); i++) {
        ChannelResult channelResult = compressChannel(
            inputChannels[i], wf, config, profiles[i]);
        result.channels.push_back(channelResult);
        reconstructedChannels.push_back(channelResult.reconstructed);
        result.compressedBytes += channelResult.compressedBytes;
    }

    result.reconstructed = mergeChannels(
        reconstructedChannels, config.colorMode, image.size());

    result.originalBytes = image.total() * image.elemSize();
    result.compressionRatio = compressionRatio(result.originalBytes, result.compressedBytes);

    if (config.colorMode == GRAY) {
        result.psnr = result.channels.front().psnr;
    } else {
        result.psnr = colorPSNR(result.original, result.reconstructed);
    }

    return result;
}

// Sikistirma sonucunu (PSNR, oran, kanal detaylari) konsola yazdirir.
void printResult(const CompressionResult& result)
{
    const auto& cfg = result.config;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n=== Compression Result ===\n";
    std::cout << "Wavelet      : ";
    switch (cfg.wavelet) {
        case HAAR: std::cout << "HAAR"; break;
        case DB2: std::cout << "DB2"; break;
        case DB4: std::cout << "DB4"; break;
        case SYM4: std::cout << "SYM4"; break;
    }
    std::cout << "\n";
    std::cout << "Level        : " << cfg.level << "\n";
    std::cout << "Q            : " << cfg.Q << "\n";
    std::cout << "Threshold    : " << cfg.threshold
              << (cfg.softThresh ? " (soft)" : " (hard)") << "\n";

    std::cout << "Color mode   : ";
    switch (cfg.colorMode) {
        case GRAY: std::cout << "GRAY"; break;
        case RGB: std::cout << "RGB"; break;
        case YCBCR: std::cout << "YCbCr"; break;
    }
    std::cout << "\n";

    for (size_t i = 0; i < result.channels.size(); i++) {
        const auto& ch = result.channels[i];
        std::cout << "Channel " << channelLabel(cfg.colorMode, static_cast<int>(i))
                  << " PSNR      : " << ch.psnr << " dB\n";
        std::cout << "Channel " << channelLabel(cfg.colorMode, static_cast<int>(i))
                  << " zero ratio: " << ch.zeroPct << "%\n";
        std::cout << "Channel " << channelLabel(cfg.colorMode, static_cast<int>(i))
                  << " compressed: " << ch.compressedBytes << " bytes\n";
    }

    std::cout << "Overall PSNR : " << result.psnr << " dB\n";
    std::cout << "Original size: " << result.originalBytes << " bytes\n";
    std::cout << "Compressed   : " << result.compressedBytes << " bytes\n";
    std::cout << "Ratio        : " << result.compressionRatio << "x\n";
}
