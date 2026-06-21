# MTM6107 Wavelet Tabanlı Görüntü Sıkıştırma

MTM6107 — Görüntü Sıkıştırma Teknikleri ve Standartları dersi final projesi.

Wavelet tabanlı görüntü sıkıştırma sistemi: 2B DWT, Huffman kodlama, kuantizasyon, eşikleme ve renkli görüntü desteği (RGB / YCbCr).

## Özellikler

- 1D / 2D DWT ve IDWT
- Wavelet türleri: HAAR, DB2, DB4, SYM4
- Huffman kodlama
- Kuantizasyon ve eşikleme (hard / soft)
- Çok seviyeli DWT
- Gri, RGB ve YCbCr modları
- PSNR ve sıkıştırma oranı metrikleri
- Toplu deney aracı (CSV + görsel çıktı)

## Gereksinimler

- C++17 derleyici
- OpenCV 4.x
- CMake 3.16+

macOS:

```bash
brew install opencv cmake
```

## Derleme

```bash
cmake -S . -B build
cmake --build build
```

Alternatif (tek komut):

```bash
g++ -std=c++17 src/main.cpp src/wavelet.cpp src/huffman.cpp src/metrics.cpp src/color.cpp src/compress.cpp \
    -Isrc $(pkg-config --cflags --libs opencv4) -o wavelet_compress
```

## Testler

Google Test tabanlı unit testler varsayılan olarak derlenir:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Yalnızca uygulamayı derlemek için:

```bash
cmake -S . -B build -DBUILD_TESTING=OFF
cmake --build build
```

Test kapsamı: DWT/IDWT, Huffman, metrikler, renk dönüşümleri ve sıkıştırma pipeline'ı.

## Çalıştırma

Test görüntüleri `data/` klasöründedir (`lena.bmp`, `lena.png`).

```bash
# Gri görüntü
./build/wavelet_compress --input data/lena.bmp --mode gray --wavelet SYM4 --level 2

# Renkli RGB
./build/wavelet_compress --input data/lena.png --mode rgb --wavelet SYM4 --level 2

# Renkli YCbCr
./build/wavelet_compress --input data/lena.png --mode ycbcr --wavelet SYM4 --level 2

# Rekonstrüksiyonu kaydet
./build/wavelet_compress --input data/lena.bmp --output results/test.png

# Toplu deneyler (D1–D4)
./build/wavelet_experiments results
```

## Parametreler

| Parametre | Açıklama | Varsayılan |
|-----------|----------|------------|
| `--input` | Giriş görüntüsü | `data/lena.bmp` |
| `--output` | Rekonstrüksiyon kayıt yolu | — |
| `--wavelet` | HAAR, DB2, DB4, SYM4 | SYM4 |
| `--mode` | gray, rgb, ycbcr | gray |
| `--level` | DWT seviyesi | 2 |
| `--Q` | Kuantizasyon adımı | 10 |
| `--threshold` | Eşik değeri | 5 |
| `--soft` | Soft thresholding | kapalı |
| `--show` | Görsel pencere | kapalı |
| `--show-levels` | DWT alt bant görseli | kapalı |

## Proje Yapısı

```
MTM6107-wavelet/
├── src/
│   ├── main.cpp           # CLI uygulaması
│   ├── experiments.cpp    # Toplu deney koşuları
│   ├── wavelet.cpp        # DWT / IDWT, kuantizasyon, eşikleme
│   ├── huffman.cpp        # Huffman encode
│   ├── color.cpp          # RGB / YCbCr dönüşümleri
│   ├── metrics.cpp        # PSNR, sıkıştırma oranı
│   └── compress.cpp       # Sıkıştırma pipeline'ı
├── data/                  # Test görüntüleri
├── tests/                 # Unit testler (Google Test)
├── results/               # Deney çıktıları (CSV, görseller)
├── CMakeLists.txt
└── README.md
```

## Deney Çıktıları

`./build/wavelet_experiments results` komutu şunları üretir:

- `results/deney_sonuclari.csv` — PSNR, sıkıştırma oranı, sıfır katsayı oranı
- `results/deney_ozet.md` — özet tablo
- `results/images/` — rekonstrüksiyon görselleri

## Örnek Deney Komutları

```bash
# D1: Wavelet karşılaştırması
for w in HAAR DB2 DB4 SYM4; do
  ./build/wavelet_compress --input data/lena.bmp --wavelet $w --level 2
done

# D2: Seviye karşılaştırması
for l in 1 2 3; do
  ./build/wavelet_compress --input data/lena.bmp --level $l
done

# D4: RGB vs YCbCr
./build/wavelet_compress --input data/lena.png --mode rgb
./build/wavelet_compress --input data/lena.png --mode ycbcr
```
