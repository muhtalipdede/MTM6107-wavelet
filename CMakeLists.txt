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

option(BUILD_TESTING "Build unit tests" ON)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(wavelet_tests
        tests/test_wavelet.cpp
        tests/test_huffman.cpp
        tests/test_metrics.cpp
        tests/test_color.cpp
        tests/test_compress.cpp
        ${WAVELET_CORE_SOURCES}
    )

    target_include_directories(wavelet_tests PRIVATE src)
    target_link_libraries(wavelet_tests PRIVATE GTest::gtest_main ${OpenCV_LIBS})

    include(GoogleTest)
    gtest_discover_tests(wavelet_tests)
endif()
