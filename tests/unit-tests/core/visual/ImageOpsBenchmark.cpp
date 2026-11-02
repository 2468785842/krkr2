#include "imageops/ImageOps.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <charconv>
#include <string_view>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace TVPImageOps;

namespace {
    volatile unsigned sink = 0;
    template <typename Function>
    double measure(Function function) {
        constexpr int iterations = 100;
        const auto start = std::chrono::steady_clock::now();
        for(int i = 0; i < iterations; ++i)
            function();
        return std::chrono::duration<double, std::micro>(
                   std::chrono::steady_clock::now() - start)
                   .count() /
            iterations;
    }

    template <typename Before, typename After>
    void compare(const char *name, Before before, After after, bool comma) {
        before();
        after();
        std::array<double, 3> a, b;
        for(int i = 0; i < 3; ++i) {
            if(i % 2) {
                b[i] = measure(after);
                a[i] = measure(before);
            } else {
                a[i] = measure(before);
                b[i] = measure(after);
            }
        }
        std::cout << (comma ? ",\n" : "\n") << "{\"name\":\"" << name
                  << "\",\"direct_us\":[" << a[0] << ',' << a[1] << ',' << a[2]
                  << "],\"imageops_us\":[" << b[0] << ',' << b[1] << ',' << b[2]
                  << "]}";
    }
} // namespace

int main(int argc, char **argv) {
    int threads = 1;
    if(argc > 2)
        return 2;
    if(argc == 2) {
        const std::string_view argument(argv[1]);
        const auto parsed = std::from_chars(
            argument.data(), argument.data() + argument.size(), threads);
        if(parsed.ec != std::errc{} ||
           parsed.ptr != argument.data() + argument.size() || threads < 1)
            return 2;
    }
    cv::setNumThreads(threads);
    constexpr int w = 320, h = 240, pitch = w * 4 + 16;
    std::vector<std::uint8_t> src(pitch * h), dst(pitch * h);
    for(std::size_t i = 0; i < src.size(); ++i)
        src[i] = static_cast<std::uint8_t>(i * 31);
    const ConstImageView source{ src.data(), w, h, pitch, PixelFormat::RGBA8 };
    const ImageView destination{ dst.data(), w, h, pitch, PixelFormat::RGBA8 };
    cv::Mat oldSource(h, w, CV_8UC4, src.data(), pitch),
        oldDestination(h, w, CV_8UC4, dst.data(), pitch);
    cv::Mat oldScaled(157, 217, CV_8UC4, dst.data(), pitch);
    const ImageView scaled{ dst.data(), 217, 157, pitch, PixelFormat::RGBA8 };
    std::cout << std::fixed << std::setprecision(3) << "{\"opencv\":\""
              << CV_VERSION << "\",\"threads\":" << threads
              << ",\"iterations\":100,\"rounds\":3,\"cases\":[";
    compare(
        "resize_nearest_rgba",
        [&] {
            cv::resize(oldSource, oldScaled, { 217, 157 }, 0, 0,
                       cv::INTER_NEAREST);
            sink += dst[0];
        },
        [&] {
            Resize(source, scaled, Interpolation::Nearest);
            sink += dst[0];
        },
        false);
    compare(
        "resize_linear_rgba",
        [&] {
            cv::resize(oldSource, oldScaled, { 217, 157 }, 0, 0,
                       cv::INTER_LINEAR);
            sink += dst[0];
        },
        [&] {
            Resize(source, scaled, Interpolation::Linear);
            sink += dst[0];
        },
        true);
    compare(
        "box_9x9_rgba",
        [&] {
            cv::boxFilter(oldSource, oldDestination, -1, { 9, 9 });
            sink += dst[0];
        },
        [&] {
            BoxBlur(source, destination, 9, 9);
            sink += dst[0];
        },
        true);
    const cv::Point2f a[] = {
        { 0, 0 }, { w - 1, 0 }, { w - 1, h - 1 }, { 0, h - 1 }
    };
    const cv::Point2f b[] = {
        { 1, 2 }, { w - 2, 1 }, { w - 3, h - 2 }, { 2, h - 3 }
    };
    const Quad from{ Point(a[0].x, a[0].y), Point(a[1].x, a[1].y),
                     Point(a[2].x, a[2].y), Point(a[3].x, a[3].y) };
    const Quad to{ Point(b[0].x, b[0].y), Point(b[1].x, b[1].y),
                   Point(b[2].x, b[2].y), Point(b[3].x, b[3].y) };
    compare(
        "warp_affine_rgba",
        [&] {
            cv::Mat output;
            cv::warpAffine(oldSource, output, cv::getAffineTransform(a, b),
                           { w, h }, cv::INTER_LINEAR);
            sink += output.ptr()[0];
        },
        [&] {
            auto output =
                WarpAffine(source, w, h, from, to, Interpolation::Linear);
            sink += output.data()[0];
        },
        true);
    compare(
        "warp_perspective_rgba",
        [&] {
            cv::Mat output;
            cv::warpPerspective(oldSource, output,
                                cv::getPerspectiveTransform(a, b), { w, h },
                                cv::INTER_LINEAR);
            sink += output.ptr()[0];
        },
        [&] {
            auto output =
                WarpPerspective(source, w, h, from, to, Interpolation::Linear);
            sink += output.data()[0];
        },
        true);
    constexpr int bigW = 3840, bigH = 2160, bigPitch = bigW * 4 + 16;
    std::vector<std::uint8_t> largeSource(bigPitch * bigH),
        largeDestination((1920 * 4 + 16) * 1080);
    for(std::size_t i = 0; i < largeSource.size(); ++i)
        largeSource[i] = static_cast<std::uint8_t>(i * 31);
    cv::Mat oldLargeSource(bigH, bigW, CV_8UC4, largeSource.data(), bigPitch),
        oldLargeDestination(1080, 1920, CV_8UC4, largeDestination.data(),
                            1920 * 4 + 16);
    compare(
        "resize_nearest_4k_to_1080p_rgba",
        [&] {
            cv::resize(oldLargeSource, oldLargeDestination, { 1920, 1080 }, 0,
                       0, cv::INTER_NEAREST);
            sink += largeDestination[0];
        },
        [&] {
            Resize({ largeSource.data(), bigW, bigH, bigPitch,
                     PixelFormat::RGBA8 },
                   { largeDestination.data(), 1920, 1080, 1920 * 4 + 16,
                     PixelFormat::RGBA8 },
                   Interpolation::Nearest);
            sink += largeDestination[0];
        },
        true);
    std::cout << "\n]}\n";
}
