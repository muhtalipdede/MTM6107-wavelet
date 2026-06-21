#include "wavelet.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// Secilen wavelet turune ait dekompozisyon ve rekonstruksiyon filtre katsayilarini dondurur.
WaveletFilter getWavelet(WaveletType type)
{
    if (type == HAAR) {
        double s = 1.0 / std::sqrt(2.0);
        return {{s, s}, {s, -s}, {s, s}, {s, -s}};
    }

    if (type == DB2) {
        double a = -0.12940952255092145;
        double b = 0.22414386804185735;
        double c = 0.836516303737469;
        double d = 0.48296291314469025;
        return {{a, b, c, d}, {-d, c, -b, -a}, {d, c, b, a}, {a, -b, c, -d}};
    }

    if (type == DB4) {
        return {
            {0.230377, 0.714846, 0.630881, -0.027983, -0.187034, 0.030841, 0.032883, -0.010597},
            {-0.010597, -0.032883, 0.030841, 0.187034, -0.027983, -0.630881, 0.714846, -0.230377},
            {-0.010597, 0.032883, 0.030841, -0.187034, -0.027983, 0.630881, 0.714846, 0.230377},
            {0.230377, -0.714846, 0.630881, 0.027983, -0.187034, -0.030841, 0.032883, 0.010597}
        };
    }

    if (type == SYM4) {
        return {
            {-0.0757657, -0.0296355, 0.4976186, 0.8037387, 0.2978577, -0.0992195, -0.0126039, 0.0322231},
            {-0.0322231, -0.0126039, 0.0992195, 0.2978577, -0.8037387, 0.4976186, 0.0296355, -0.0757657},
            {0.0322231, -0.0126039, -0.0992195, 0.2978577, 0.8037387, 0.4976186, -0.0296355, -0.0757657},
            {-0.0757657, 0.0296355, 0.4976186, -0.8037387, 0.2978577, 0.0992195, -0.0126039, -0.0322231}
        };
    }

    return {};
}

// Komut satirindan gelen wavelet adini enum degerine cevirir.
WaveletType parseWavelet(const std::string& name)
{
    if (name == "HAAR" || name == "haar") return HAAR;
    if (name == "DB2" || name == "db2") return DB2;
    if (name == "DB4" || name == "db4") return DB4;
    if (name == "SYM4" || name == "sym4") return SYM4;
    throw std::invalid_argument("Unknown wavelet: " + name);
}

// 1B DWT: vektoru yaklasik (Lo) ve detay (Hi) bantlarina ayirir.
//   A[i] = sum_k x[(2i+k) mod n] * Lo[k]
//   D[i] = sum_k x[(2i+k) mod n] * Hi[k]
void dwt1D(std::vector<double>& data,
           const std::vector<double>& Lo,
           const std::vector<double>& Hi)
{
    int n = static_cast<int>(data.size());
    int f = static_cast<int>(Lo.size());
    std::vector<double> temp(n, 0.0);

    for (int i = 0; i < n / 2; i++) {
        for (int k = 0; k < f; k++) {
            int idx = (2 * i + k) % n;
            temp[i] += data[idx] * Lo[k];
            temp[n / 2 + i] += data[idx] * Hi[k];
        }
    }

    data = temp;
}

// 1B IDWT: yaklasik ve detay katsayilarindan orijinal vektoru geri olusturur.
//   x[(2i+k) mod n] += A[i]*Lo[k] + D[i]*Hi[k]
void idwt1D(std::vector<double>& data,
            const std::vector<double>& Lo,
            const std::vector<double>& Hi)
{
    int n = static_cast<int>(data.size());
    int f = static_cast<int>(Lo.size());
    std::vector<double> temp(n, 0.0);

    for (int i = 0; i < n / 2; i++) {
        for (int k = 0; k < f; k++) {
            int idx = (2 * i + k) % n;
            temp[idx] += data[i] * Lo[k];
            temp[idx] += data[n / 2 + i] * Hi[k];
        }
    }

    data = temp;
}

// 2B DWT: once satirlara, sonra sutunlara 1B DWT uygular; cok seviyeli ayrismayi destekler.
//   Her seviyede alt bantlar: LL (yaklasik), LH, HL, HH (detay)
void dwt2D(cv::Mat& img, const WaveletFilter& wf, int levels)
{
    for (int lev = 0; lev < levels; lev++) {
        int r = img.rows >> lev;
        int c = img.cols >> lev;

        for (int i = 0; i < r; i++) {
            std::vector<double> row(c);
            for (int j = 0; j < c; j++) {
                row[j] = img.at<double>(i, j);
            }
            dwt1D(row, wf.Lo_D, wf.Hi_D);
            for (int j = 0; j < c; j++) {
                img.at<double>(i, j) = row[j];
            }
        }

        for (int j = 0; j < c; j++) {
            std::vector<double> col(r);
            for (int i = 0; i < r; i++) {
                col[i] = img.at<double>(i, j);
            }
            dwt1D(col, wf.Lo_D, wf.Hi_D);
            for (int i = 0; i < r; i++) {
                img.at<double>(i, j) = col[i];
            }
        }
    }
}

// 2B IDWT: en yuksek seviyeden baslayarak sutun ve satir bazli ters donusum uygular.
void idwt2D(cv::Mat& img, const WaveletFilter& wf, int levels)
{
    for (int lev = levels - 1; lev >= 0; lev--) {
        int r = img.rows >> lev;
        int c = img.cols >> lev;

        for (int j = 0; j < c; j++) {
            std::vector<double> col(r);
            for (int i = 0; i < r; i++) {
                col[i] = img.at<double>(i, j);
            }
            idwt1D(col, wf.Lo_R, wf.Hi_R);
            for (int i = 0; i < r; i++) {
                img.at<double>(i, j) = col[i];
            }
        }

        for (int i = 0; i < r; i++) {
            std::vector<double> row(c);
            for (int j = 0; j < c; j++) {
                row[j] = img.at<double>(i, j);
            }
            idwt1D(row, wf.Lo_R, wf.Hi_R);
            for (int j = 0; j < c; j++) {
                img.at<double>(i, j) = row[j];
            }
        }
    }
}

// Wavelet katsayilarini Q adimina gore yuvarlayarak kayip sikistirma uygular.
//   w' = round(w / Q) * Q
void quantize(cv::Mat& coeff, double Q)
{
    if (Q <= 0.0) return;

    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            double val = coeff.at<double>(i, j);
            coeff.at<double>(i, j) = std::round(val / Q) * Q;
        }
    }
}

// Mutlak degeri T'den kucuk katsayilari sifirlar (hard thresholding).
//   w' = w,  |w| >= T
//   w' = 0,  |w| <  T
void hardThreshold(cv::Mat& coeff, double T)
{
    if (T <= 0.0) return;

    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            if (std::abs(coeff.at<double>(i, j)) < T) {
                coeff.at<double>(i, j) = 0.0;
            }
        }
    }
}

// Esik alti katsayilari sifirlar, usttekileri T kadar kucultur (soft thresholding).
//   w' = sign(w) * max(|w| - T, 0)
void softThreshold(cv::Mat& coeff, double T)
{
    if (T <= 0.0) return;

    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            double val = coeff.at<double>(i, j);
            if (std::abs(val) < T) {
                coeff.at<double>(i, j) = 0.0;
            } else {
                coeff.at<double>(i, j) = (val > 0 ? 1.0 : -1.0) * (std::abs(val) - T);
            }
        }
    }
}

// Katsayi matrisindeki sifir elemanlarin oranini hesaplar (Huffman verimliligi icin).
//   Z = (# { w_ij = 0 }) / (rows * cols)
double zeroRatio(const cv::Mat& coeff)
{
    int zeros = 0;
    for (int i = 0; i < coeff.rows; i++) {
        for (int j = 0; j < coeff.cols; j++) {
            if (coeff.at<double>(i, j) == 0.0) {
                zeros++;
            }
        }
    }
    return static_cast<double>(zeros) / coeff.total();
}

// Goruntu boyutunun secilen DWT seviyesi icin yeterli olup olmadigini kontrol eder.
bool isValidSize(int rows, int cols, int level)
{
    int minDim = std::min(rows, cols);
    return minDim >= (1 << level);
}

// Belirli seviyedeki LL, LH, HL, HH alt bantlarini 2x2 blok olarak gorsellestirir.
cv::Mat showLevel(const cv::Mat& coeff, int level)
{
    cv::Mat vis;
    cv::normalize(coeff, vis, 0, 255, cv::NORM_MINMAX);
    vis.convertTo(vis, CV_8U);

    int r = vis.rows >> level;
    int c = vis.cols >> level;
    int hr = r / 2;
    int hc = c / 2;

    cv::Mat LL = vis(cv::Rect(0, 0, hc, hr));
    cv::Mat LH = vis(cv::Rect(hc, 0, hc, hr));
    cv::Mat HL = vis(cv::Rect(0, hr, hc, hr));
    cv::Mat HH = vis(cv::Rect(hc, hr, hc, hr));

    cv::Mat top, bottom, out;
    cv::hconcat(LL, LH, top);
    cv::hconcat(HL, HH, bottom);
    cv::vconcat(top, bottom, out);
    return out;
}
