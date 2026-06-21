#include "compress.h"
#include "wavelet.h"

#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <string>

namespace {

struct AppOptions {
    std::string input = "data/lena.bmp";
    std::string output;
    std::string wavelet = "SYM4";
    std::string mode = "gray";
    int level = 2;
    double Q = 10.0;
    double threshold = 5.0;
    bool softThresh = false;
    bool show = false;
    bool showLevels = false;
};

// Komut satiri kullanim bilgisini yazdirir.
void printUsage(const char* program)
{
    std::cout
        << "Usage: " << program << " [options]\n\n"
        << "Options:\n"
        << "  --input <path>        Input image path (default: data/lena.bmp)\n"
        << "  --output <path>       Save reconstructed image\n"
        << "  --wavelet <type>      HAAR | DB2 | DB4 | SYM4 (default: SYM4)\n"
        << "  --mode <mode>         gray | rgb | ycbcr (default: gray)\n"
        << "  --level <n>           DWT decomposition level (default: 2)\n"
        << "  --Q <value>           Quantization step (default: 10)\n"
        << "  --threshold <value>   Threshold value (default: 5)\n"
        << "  --soft                Use soft thresholding\n"
        << "  --show                Display original and reconstructed windows\n"
        << "  --show-levels         Display DWT subband visualization\n"
        << "  --help                Show this help message\n\n"
        << "Examples:\n"
        << "  " << program << " --input data/lena.bmp --mode gray --wavelet SYM4 --level 2\n"
        << "  " << program << " --input data/lena.png --mode rgb --wavelet SYM4 --level 2\n"
        << "  " << program << " --input data/lena.png --mode ycbcr --Q 10 --threshold 5\n";
}

// argc/argv uzerinden uygulama parametrelerini okur.
AppOptions parseArgs(int argc, char** argv)
{
    AppOptions opts;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (arg == "--input" && i + 1 < argc) {
            opts.input = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            opts.output = argv[++i];
        } else if (arg == "--wavelet" && i + 1 < argc) {
            opts.wavelet = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            opts.mode = argv[++i];
        } else if (arg == "--level" && i + 1 < argc) {
            opts.level = std::stoi(argv[++i]);
        } else if (arg == "--Q" && i + 1 < argc) {
            opts.Q = std::stod(argv[++i]);
        } else if (arg == "--threshold" && i + 1 < argc) {
            opts.threshold = std::stod(argv[++i]);
        } else if (arg == "--soft") {
            opts.softThresh = true;
        } else if (arg == "--show") {
            opts.show = true;
        } else if (arg == "--show-levels") {
            opts.showLevels = true;
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    return opts;
}

// Renk moduna uygun bayraklarla goruntuyu diskten yukler.
cv::Mat loadImage(const std::string& path, ColorMode mode)
{
    int flags = (mode == GRAY) ? cv::IMREAD_GRAYSCALE : cv::IMREAD_COLOR;
    cv::Mat image = cv::imread(path, flags);
    if (image.empty()) {
        throw std::runtime_error("Could not load image: " + path);
    }
    return image;
}

}  // namespace

// Ana program: parametreleri okur, sikistirma yapar ve sonuclari gosterir/kaydeder.
int main(int argc, char** argv)
{
    try {
        AppOptions opts = parseArgs(argc, argv);
        CompressionConfig config;
        config.wavelet = parseWavelet(opts.wavelet);
        config.colorMode = parseColorMode(opts.mode);
        config.level = opts.level;
        config.Q = opts.Q;
        config.threshold = opts.threshold;
        config.softThresh = opts.softThresh;

        cv::Mat image = loadImage(opts.input, config.colorMode);
        CompressionResult result = compressImage(image, config);
        printResult(result);

        if (!opts.output.empty()) {
            if (cv::imwrite(opts.output, result.reconstructed)) {
                std::cout << "Saved reconstructed image to: " << opts.output << "\n";
            } else {
                std::cerr << "Failed to save image: " << opts.output << "\n";
            }
        }

        if (opts.showLevels && config.colorMode == GRAY) {
            WaveletFilter wf = getWavelet(config.wavelet);
            cv::Mat coeff;
            image.convertTo(coeff, CV_64F);
            dwt2D(coeff, wf, config.level);

            for (int i = 0; i < config.level; i++) {
                cv::imshow("Level " + std::to_string(i + 1), showLevel(coeff, i));
            }
        }

        if (opts.show) {
            cv::imshow("Original", result.original);
            cv::imshow("Reconstructed", result.reconstructed);
            cv::waitKey(0);
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
