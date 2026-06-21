cmake_minimum_required(VERSION 3.16)
project(MTM6107_wavelet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

set(WAVELET_CORE_SOURCES
    src/wavelet.cpp
    src/huffman.cpp
    src/metrics.cpp
    src/color.cpp
    src/compress.cpp
)

add_executable(wavelet_compress
    src/main.cpp
    ${WAVELET_CORE_SOURCES}
)

add_executable(wavelet_experiments
    src/experiments.cpp
    ${WAVELET_CORE_SOURCES}
)

foreach(target wavelet_compress wavelet_experiments)
    target_include_directories(${target} PRIVATE src)
    target_link_libraries(${target} PRIVATE ${OpenCV_LIBS})
endforeach()
