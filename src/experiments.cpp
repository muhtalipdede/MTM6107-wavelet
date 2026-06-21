#include "compress.h"
#include "color.h"
#include "wavelet.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct ExperimentRow {
    std::string id;
    std::string group;
    std::string image;
    std::string wavelet;
    int level = 0;
    double Q = 0.0;
    double threshold = 0.0;
    std::string colorMode;
    double psnr = 0.0;
    double compressionRatio = 0.0;
    double zeroPct = 0.0;
    size_t originalBytes = 0;
    size_t compressedBytes = 0;
    std::string outputImage;
};

// Wavelet enum degerini rapor/CSV icin metin olarak dondurur.
std::string waveletName(WaveletType type)
{
    switch (type) {
        case HAAR: return "HAAR";
        case DB2: return "DB2";
        case DB4: return "DB4";
        case SYM4: return "SYM4";
    }
    return "UNKNOWN";
}

// Renk modu enum degerini metin olarak dondurur.
std::string colorModeName(ColorMode mode)
{
    switch (mode) {
        case GRAY: return "gray";
        case RGB: return "rgb";
        case YCBCR: return "ycbcr";
    }
    return "unknown";
}

// Deneyler icin goruntuyu renk moduna gore yukler.
cv::Mat loadImage(const std::string& path, ColorMode mode)
{
    int flags = (mode == GRAY) ? cv::IMREAD_GRAYSCALE : cv::IMREAD_COLOR;
    cv::Mat image = cv::imread(path, flags);
    if (image.empty()) {
        throw std::runtime_error("Could not load image: " + path);
    }
    return image;
}

// Tum kanallarin ortalama sifir katsayi oranini hesaplar.
double averageZeroRatio(const CompressionResult& result)
{
    if (result.channels.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& ch : result.channels) {
        sum += ch.zeroPct;
    }
    return sum / static_cast<double>(result.channels.size());
}

// Tek bir deney kosusunu calistirir ve metrikleri ExperimentRow olarak dondurur.
ExperimentRow runSingle(const std::string& id,
                        const std::string& group,
                        const std::string& imagePath,
                        const CompressionConfig& config,
                        const std::string& imageOutput)
{
    cv::Mat image = loadImage(imagePath, config.colorMode);
    CompressionResult result = compressImage(image, config);

    if (!imageOutput.empty()) {
        fs::create_directories(fs::path(imageOutput).parent_path());
        if (!cv::imwrite(imageOutput, result.reconstructed)) {
            throw std::runtime_error("Failed to save image: " + imageOutput);
        }
    }

    ExperimentRow row;
    row.id = id;
    row.group = group;
    row.image = fs::path(imagePath).filename().string();
    row.wavelet = waveletName(config.wavelet);
    row.level = config.level;
    row.Q = config.Q;
    row.threshold = config.threshold;
    row.colorMode = colorModeName(config.colorMode);
    row.psnr = result.psnr;
    row.compressionRatio = result.compressionRatio;
    row.zeroPct = averageZeroRatio(result);
    row.originalBytes = result.originalBytes;
    row.compressedBytes = result.compressedBytes;
    row.outputImage = imageOutput;
    return row;
}

// Deney sonuclarini CSV dosyasina yazar.
void writeCsv(const std::vector<ExperimentRow>& rows, const std::string& path)
{
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not write CSV: " + path);
    }

    file << "id,group,image,wavelet,level,Q,threshold,color_mode,psnr_db,compression_ratio,zero_pct,original_bytes,compressed_bytes,output_image\n";
    file << std::fixed << std::setprecision(4);
    for (const auto& row : rows) {
        file << row.id << ','
             << row.group << ','
             << row.image << ','
             << row.wavelet << ','
             << row.level << ','
             << row.Q << ','
             << row.threshold << ','
             << row.colorMode << ','
             << row.psnr << ','
             << row.compressionRatio << ','
             << row.zeroPct << ','
             << row.originalBytes << ','
             << row.compressedBytes << ','
             << row.outputImage << '\n';
    }
}

// Deney sonuclarinin ozet tablosunu Markdown olarak yazar.
void writeSummaryMarkdown(const std::vector<ExperimentRow>& rows, const std::string& path)
{
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not write summary: " + path);
    }

    file << "# Deneysel Analiz Ozet Tablosu\n\n";
    file << "Kaynak: `wavelet_experiments`, `results/deney_sonuclari.csv`\n\n";
    file << "| ID | Grup | Wavelet | Level | Mode | PSNR (dB) | CR (x) | Sifir (%) |\n";
    file << "|----|------|---------|-------|------|-----------|--------|----------|\n";

    file << std::fixed << std::setprecision(2);
    for (const auto& row : rows) {
        file << "| " << row.id
             << " | " << row.group
             << " | " << row.wavelet
             << " | " << row.level
             << " | " << row.colorMode
             << " | " << row.psnr
             << " | " << row.compressionRatio
             << " | " << row.zeroPct
             << " |\n";
    }
}

// Tum deneylerde kullanilacak varsayilan sikistirma parametrelerini dondurur.
CompressionConfig baseConfig()
{
    CompressionConfig config;
    config.wavelet = SYM4;
    config.level = 2;
    config.Q = 10.0;
    config.threshold = 5.0;
    config.softThresh = false;
    config.colorMode = GRAY;
    return config;
}

// Giris goruntusu dosyasinin varligini kontrol eder.
void ensureInputExists(const std::string& path)
{
    if (!fs::exists(path)) {
        throw std::runtime_error("Missing input image: " + path);
    }
}

}  // namespace

// D1-D4 deney gruplarini calistirir; CSV, ozet tablo ve gorselleri uretir.
int main(int argc, char** argv)
{
    try {
        std::string grayImage = "data/lena.bmp";
        std::string colorImage = "data/lena.png";
        std::string resultsDir = "results";

        if (argc >= 2) {
            resultsDir = argv[1];
        }

        ensureInputExists(grayImage);
        ensureInputExists(colorImage);

        std::vector<ExperimentRow> rows;
        const std::string imageDir = resultsDir + "/images";

        // D1: Wavelet turu karsilastirmasi
        {
            WaveletType wavelets[] = {HAAR, DB2, DB4, SYM4};
            for (WaveletType w : wavelets) {
                CompressionConfig config = baseConfig();
                config.wavelet = w;
                config.colorMode = GRAY;

                std::string id = "D1-" + waveletName(w);
                std::string output = imageDir + "/d1_" + waveletName(w) + "_gray.png";
                rows.push_back(runSingle(id, "D1_wavelet", grayImage, config, output));
            }
        }

        // D2: Decomposition seviyesi karsilastirmasi
        for (int level : {1, 2, 3}) {
            CompressionConfig config = baseConfig();
            config.level = level;
            config.colorMode = GRAY;

            std::string id = "D2-L" + std::to_string(level);
            std::string output = imageDir + "/d2_level" + std::to_string(level) + "_gray.png";
            rows.push_back(runSingle(id, "D2_level", grayImage, config, output));
        }

        // D3: Gri vs renkli
        {
            CompressionConfig grayConfig = baseConfig();
            grayConfig.colorMode = GRAY;
            rows.push_back(runSingle("D3-gray", "D3_color", grayImage, grayConfig,
                                     imageDir + "/d3_gray.png"));

            CompressionConfig rgbConfig = baseConfig();
            rgbConfig.colorMode = RGB;
            rows.push_back(runSingle("D3-rgb", "D3_color", colorImage, rgbConfig,
                                     imageDir + "/d3_rgb.png"));

            CompressionConfig ycbcrConfig = baseConfig();
            ycbcrConfig.colorMode = YCBCR;
            rows.push_back(runSingle("D3-ycbcr", "D3_color", colorImage, ycbcrConfig,
                                     imageDir + "/d3_ycbcr.png"));
        }

        // D4: RGB vs YCbCr
        {
            CompressionConfig rgbConfig = baseConfig();
            rgbConfig.colorMode = RGB;
            rows.push_back(runSingle("D4-rgb", "D4_colorspace", colorImage, rgbConfig,
                                     imageDir + "/d4_rgb.png"));

            CompressionConfig ycbcrConfig = baseConfig();
            ycbcrConfig.colorMode = YCBCR;
            rows.push_back(runSingle("D4-ycbcr", "D4_colorspace", colorImage, ycbcrConfig,
                                     imageDir + "/d4_ycbcr.png"));
        }

        const std::string csvPath = resultsDir + "/deney_sonuclari.csv";
        const std::string mdPath = resultsDir + "/deney_ozet.md";
        writeCsv(rows, csvPath);
        writeSummaryMarkdown(rows, mdPath);

        std::cout << "Experiments completed.\n";
        std::cout << "CSV   : " << csvPath << "\n";
        std::cout << "Summary: " << mdPath << "\n";
        std::cout << "Images : " << imageDir << "/\n";
        std::cout << "Total runs: " << rows.size() << "\n";

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
