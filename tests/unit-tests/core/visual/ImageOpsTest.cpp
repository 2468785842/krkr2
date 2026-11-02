#include "imageops/ImageOps.h"

#include <catch2/catch_test_macros.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace TVPImageOps;

namespace {
    // 前后哨兵及行间 padding
    // 用来检查视图外写入；输入使用非均匀、非预乘的四通道像素。
    struct Pixels {
        int width, height, channels;
        std::ptrdiff_t pitch;
        std::vector<std::uint8_t> bytes;

        Pixels(int w, int h, int c, int padding = 13) :
            width(w), height(h), channels(c), pitch(w * c + padding),
            bytes(32 + pitch * h, 0xa5) {}

        std::uint8_t *data() { return bytes.data() + 16; }
        ImageView view() {
            return { data(), width, height, pitch,
                     channels == 1 ? PixelFormat::Gray8 : PixelFormat::RGBA8 };
        }
        cv::Mat mat() {
            return { height, width, CV_MAKETYPE(CV_8U, channels), data(),
                     static_cast<size_t>(pitch) };
        }
        void pattern() {
            for(int y = 0; y < height; ++y)
                for(int x = 0; x < width * channels; ++x)
                    data()[y * pitch + x] = static_cast<std::uint8_t>(
                        (x * 31 + y * 57 + x * y) & 255);
        }
    };

    constexpr Interpolation modes[] = { Interpolation::Nearest,
                                        Interpolation::Area,
                                        Interpolation::Linear,
                                        Interpolation::Cubic };
    constexpr int oldFlags[] = { cv::INTER_NEAREST, cv::INTER_AREA,
                                 cv::INTER_LINEAR, cv::INTER_CUBIC };

    std::array<cv::Point2f, 4> oldPoints(const Quad &quad) {
        return { cv::Point2f(quad[0].x, quad[0].y),
                 cv::Point2f(quad[1].x, quad[1].y),
                 cv::Point2f(quad[2].x, quad[2].y),
                 cv::Point2f(quad[3].x, quad[3].y) };
    }

    void same(const ImageBuffer &actual, const cv::Mat &expected) {
        REQUIRE(actual.width() == expected.cols);
        REQUIRE(actual.height() == expected.rows);
        REQUIRE(
            actual.pitch() ==
            static_cast<std::ptrdiff_t>(expected.cols * expected.channels()));
        for(int y = 0; y < expected.rows; ++y)
            REQUIRE(std::equal(actual.data() + y * actual.pitch(),
                               actual.data() + (y + 1) * actual.pitch(),
                               expected.ptr(y)));
    }
} // namespace

TEST_CASE("ImageOps resize matches the former renderer for every mode and byte "
          "format",
          "[imageops]") {
    const int sizes[][4] = { { 1, 1, 7, 3 }, { 7, 3, 1, 1 }, { 5, 7, 8, 3 },
                             { 8, 8, 3, 5 }, { 1, 5, 1, 9 }, { 5, 1, 9, 1 } };
    for(int channels : { 1, 4 }) {
        for(const auto &size : sizes) {
            Pixels source(size[0], size[1], channels);
            source.pattern();
            const auto original = source.bytes;
            for(int mode = 0; mode < 4; ++mode) {
                Pixels actual(size[2], size[3], channels, 17),
                    expected = actual;
                auto src = source.mat(), dst = expected.mat();
                cv::resize(src, dst, dst.size(), 0, 0, oldFlags[mode]);
                Resize(source.view(), actual.view(), modes[mode]);
                REQUIRE(actual.bytes == expected.bytes);
                REQUIRE(source.bytes == original);
            }
        }
    }
}

TEST_CASE(
    "ImageOps nearest scaling and alpha channels have explicit expected values",
    "[imageops]") {
    const std::uint8_t source[] = { 1, 2, 3, 4 };
    std::uint8_t output[16]{};
    Resize({ source, 2, 2, 0, PixelFormat::Gray8 },
           { output, 4, 4, 0, PixelFormat::Gray8 }, Interpolation::Nearest);
    const std::uint8_t expected[] = { 1, 1, 2, 2, 1, 1, 2, 2,
                                      3, 3, 4, 4, 3, 3, 4, 4 };
    REQUIRE(
        std::equal(std::begin(output), std::end(output), std::begin(expected)));
    const std::uint8_t rgba[] = { 255, 0, 0, 0, 0, 0, 0, 255 };
    std::uint8_t blended[12]{};
    Resize({ rgba, 2, 1, 0, PixelFormat::RGBA8 },
           { blended, 3, 1, 0, PixelFormat::RGBA8 }, Interpolation::Linear);
    REQUIRE(blended[4] == 128);
    REQUIRE(blended[7] == 128);
}

TEST_CASE("ImageOps resize respects an ROI pointer and preserves surrounding "
          "destination bytes",
          "[imageops]") {
    Pixels source(9, 7, 4), actual(8, 6, 4), expected = actual;
    source.pattern();
    auto *input = source.data() + 2 * source.pitch + 3 * 4;
    auto *output = actual.data() + actual.pitch + 2 * 4;
    auto *reference = expected.data() + expected.pitch + 2 * 4;
    cv::Mat src(3, 4, CV_8UC4, input, source.pitch),
        dst(4, 5, CV_8UC4, reference, expected.pitch);
    cv::resize(src, dst, dst.size(), 0, 0, cv::INTER_CUBIC);
    Resize({ input, 4, 3, source.pitch, PixelFormat::RGBA8 },
           { output, 5, 4, actual.pitch, PixelFormat::RGBA8 },
           Interpolation::Cubic);
    REQUIRE(actual.bytes == expected.bytes);
}

TEST_CASE(
    "ImageOps nearest scaling matches legacy rounding across dimension pairs",
    "[imageops][nearest]") {
    for(int channels : { 1, 4 }) {
        for(bool vertical : { false, true }) {
            for(int from = 1; from <= 100; ++from) {
                Pixels source(vertical ? 3 : from, vertical ? from : 3,
                              channels);
                source.pattern();
                const auto original = source.bytes;
                for(int to = 1; to <= 100; ++to) {
                    CAPTURE(channels, vertical, from, to);
                    Pixels actual(vertical ? 5 : to, vertical ? to : 5,
                                  channels, 17),
                        expected = actual;
                    auto src = source.mat(), dst = expected.mat();
                    cv::resize(src, dst, dst.size(), 0, 0, cv::INTER_NEAREST);
                    Resize(source.view(), actual.view(),
                           Interpolation::Nearest);
                    REQUIRE(actual.bytes == expected.bytes);
                }
                REQUIRE(source.bytes == original);
            }
        }
    }
}

TEST_CASE("ImageOps nearest retains historical reciprocal-rounding pixels",
          "[imageops][nearest]") {
    const std::uint8_t source[] = { 10, 20, 30, 40, 50, 60 };
    std::uint8_t horizontal[34]{}, vertical[34]{};
    Resize({ source, 6, 1, 0, PixelFormat::Gray8 },
           { horizontal, 34, 1, 0, PixelFormat::Gray8 },
           Interpolation::Nearest);
    Resize({ source, 1, 6, 0, PixelFormat::Gray8 },
           { vertical, 1, 34, 0, PixelFormat::Gray8 }, Interpolation::Nearest);
    // 旧 OpenCV 先算 double(34)/6 再取倒数，17*inverse 略小于 3。
    REQUIRE(horizontal[17] == 30);
    REQUIRE(vertical[17] == 30);
    REQUIRE(horizontal[18] == 40);
    REQUIRE(vertical[18] == 40);
}

TEST_CASE(
    "ImageOps nearest preserves cropped unaligned views and identity copies",
    "[imageops][nearest]") {
    for(int channels : { 1, 4 }) {
        Pixels source(17, 13, channels), actual(19, 15, channels, 19),
            expected = actual;
        source.pattern();
        const auto original = source.bytes;
        auto *input = source.data() + source.pitch + 2 * channels;
        auto *output = actual.data() + actual.pitch + 3 * channels;
        auto *reference = expected.data() + expected.pitch + 3 * channels;
        const auto format = source.view().format;
        cv::Mat src(7, 9, CV_MAKETYPE(CV_8U, channels), input, source.pitch),
            dst(11, 13, CV_MAKETYPE(CV_8U, channels), reference,
                expected.pitch);
        cv::resize(src, dst, dst.size(), 0, 0, cv::INTER_NEAREST);
        Resize({ input, 9, 7, source.pitch, format },
               { output, 13, 11, actual.pitch, format },
               Interpolation::Nearest);
        REQUIRE(actual.bytes == expected.bytes);
        REQUIRE(source.bytes == original);
        for(int padding : { 0, 17 }) {
            Pixels copied(17, 13, channels, padding), referenceCopy = copied;
            auto srcMatrix = source.mat(), dstMatrix = referenceCopy.mat();
            cv::resize(srcMatrix, dstMatrix, dstMatrix.size(), 0, 0,
                       cv::INTER_NEAREST);
            Resize(source.view(), copied.view(), Interpolation::Nearest);
            REQUIRE(copied.bytes == referenceCopy.bytes);
            const auto snapshot = copied.bytes;
            Resize(copied.view(), copied.view(), Interpolation::Nearest);
            REQUIRE(copied.bytes == snapshot);
        }
    }
}

TEST_CASE("ImageOps nearest handles SIMD widths and multi-row images",
          "[imageops][nearest]") {
    const int sizes[][4] = { { 320, 240, 217, 157 },
                             { 384, 257, 1025, 771 },
                             { 1001, 23, 1031, 31 },
                             { 1, 1, 8193, 3 },
                             { 8193, 3, 1, 1 } };
    struct RestoreThreads {
        int previous = cv::getNumThreads();
        ~RestoreThreads() { cv::setNumThreads(previous); }
    } restoreThreads;
    for(int threads : { 1, 3 }) {
        cv::setNumThreads(threads);
        for(int channels : { 1, 4 }) {
            for(const auto &size : sizes) {
                CAPTURE(threads, channels, size[0], size[1], size[2], size[3]);
                Pixels source(size[0], size[1], channels);
                source.pattern();
                const auto original = source.bytes;
                Pixels actual(size[2], size[3], channels, 17),
                    expected = actual;
                auto src = source.mat(), dst = expected.mat();
                cv::resize(src, dst, dst.size(), 0, 0, cv::INTER_NEAREST);
                Resize(source.view(), actual.view(), Interpolation::Nearest);
                REQUIRE(actual.bytes == expected.bytes);
                REQUIRE(source.bytes == original);
            }
        }
    }
}

TEST_CASE("ImageOps box blur preserves even anchors and reflect borders "
          "including in-place use",
          "[imageops]") {
    for(int channels : { 1, 4 }) {
        for(const auto &shape :
            { cv::Size(1, 1), cv::Size(3, 5), cv::Size(8, 7) }) {
            for(const auto &kernel :
                { cv::Size(2, 2), cv::Size(3, 5), cv::Size(9, 9) }) {
                Pixels source(shape.width, shape.height, channels);
                source.pattern();
                Pixels actual(shape.width, shape.height, channels),
                    expected = actual;
                auto src = source.mat(), dst = expected.mat();
                cv::boxFilter(src, dst, -1, kernel);
                BoxBlur(source.view(), actual.view(), kernel.width,
                        kernel.height);
                REQUIRE(actual.bytes == expected.bytes);
                actual = source;
                expected = source;
                auto before = expected.mat(), after = expected.mat();
                cv::boxFilter(before, after, -1, kernel);
                BoxBlur(actual.view(), actual.view(), kernel.width,
                        kernel.height);
                REQUIRE(actual.bytes == expected.bytes);
            }
        }
    }
}

TEST_CASE("ImageOps blur boundaries belong to the cropped view", "[imageops]") {
    Pixels source(8, 7, 4), actual(8, 7, 4), expected = actual;
    source.pattern();
    auto *src = source.data() + source.pitch + 2 * 4;
    auto *dst = actual.data() + actual.pitch + 2 * 4;
    cv::Mat input(3, 4, CV_8UC4, src, source.pitch);
    cv::Mat output(3, 4, CV_8UC4, expected.data() + expected.pitch + 2 * 4,
                   expected.pitch);
    cv::boxFilter(input, output, -1, { 4, 6 });
    BoxBlur({ src, 4, 3, source.pitch, PixelFormat::RGBA8 },
            { dst, 4, 3, actual.pitch, PixelFormat::RGBA8 }, 4, 6);
    REQUIRE(actual.bytes == expected.bytes);
}

TEST_CASE("ImageOps affine and perspective warps match former coordinate and "
          "border semantics",
          "[imageops]") {
    const Quad from{ Point(0, 0), Point(6, 0), Point(6, 5), Point(0, 5) };
    const Quad targets[] = {
        from,
        { Point(2, 1), Point(8, 1), Point(8, 6), Point(2, 6) },
        { Point(-1.3f, 0.4f), Point(5.8f, -0.7f), Point(7.1f, 4.3f),
          Point(0.2f, 6.1f) },
        { Point(0, 0), Point(0, 0), Point(0, 0), Point(0, 0) }
    };
    for(int channels : { 1, 4 }) {
        Pixels source(7, 6, channels);
        source.pattern();
        const auto original = source.bytes;
        for(const auto &to : targets) {
            auto oldFrom = oldPoints(from), oldTo = oldPoints(to);
            for(int mode = 0; mode < 4; ++mode) {
                cv::Mat affine, perspective;
                auto src = source.mat();
                cv::warpAffine(
                    src, affine,
                    cv::getAffineTransform(oldFrom.data(), oldTo.data()),
                    { 9, 8 }, oldFlags[mode]);
                cv::warpPerspective(
                    src, perspective,
                    cv::getPerspectiveTransform(oldFrom.data(), oldTo.data()),
                    { 9, 8 }, oldFlags[mode]);
                same(WarpAffine(source.view(), 9, 8, from, to, modes[mode]),
                     affine);
                same(
                    WarpPerspective(source.view(), 9, 8, from, to, modes[mode]),
                    perspective);
                REQUIRE(source.bytes == original);
            }
        }
    }
}

TEST_CASE("ImageOps perspective matrix preserves float input and row-major "
          "double output",
          "[imageops]") {
    const Quad from{ Point(0, 0), Point(7, 0), Point(7, 5), Point(0, 5) };
    const Quad to{ Point(1.2f, -0.4f), Point(8.3f, 1.1f), Point(7.2f, 6.4f),
                   Point(-0.1f, 5.2f) };
    auto a = oldPoints(from), b = oldPoints(to);
    auto old = cv::getPerspectiveTransform(a.data(), b.data());
    const auto matrix = PerspectiveTransform(from, to);
    REQUIRE(std::equal(matrix.begin(), matrix.end(), old.ptr<double>()));
    for(int i = 0; i < 4; ++i) {
        const double divisor =
            matrix[6] * from[i].x + matrix[7] * from[i].y + matrix[8];
        REQUIRE(std::abs((matrix[0] * from[i].x + matrix[1] * from[i].y +
                          matrix[2]) /
                             divisor -
                         to[i].x) < 1e-5);
        REQUIRE(std::abs((matrix[3] * from[i].x + matrix[4] * from[i].y +
                          matrix[5]) /
                             divisor -
                         to[i].y) < 1e-5);
    }
}

TEST_CASE(
    "ImageOps warp results own their pixels across moves and later operations",
    "[imageops]") {
    Pixels source(3, 3, 4);
    source.pattern();
    const Quad quad{ Point(0, 0), Point(2, 0), Point(2, 2), Point(0, 2) };
    auto result = WarpPerspective(source.view(), 3, 3, quad, quad,
                                  Interpolation::Nearest);
    const std::vector<std::uint8_t> snapshot(
        result.data(), result.data() + result.pitch() * result.height());
    auto moved = std::move(result);
    for(int i = 0; i < 8; ++i)
        WarpAffine(source.view(), 7, 5, quad, quad, Interpolation::Linear);
    REQUIRE(std::equal(snapshot.begin(), snapshot.end(), moved.data()));
    for(int y = 0; y < 3; ++y)
        REQUIRE(std::equal(moved.data() + y * moved.pitch(),
                           moved.data() + (y + 1) * moved.pitch(),
                           source.data() + y * source.pitch));
}

TEST_CASE("ImageOps rejects invalid views without writing the destination",
          "[imageops]") {
    Pixels source(3, 3, 4), destination(3, 3, 4);
    const auto original = destination.bytes;
    auto valid = source.view();
    ConstImageView invalid[] = {
        { nullptr, 3, 3, 12, PixelFormat::RGBA8 },
        { source.data(), 0, 3, 12, PixelFormat::RGBA8 },
        { source.data(), 3, 0, 12, PixelFormat::RGBA8 },
        { source.data(), 3, 3, -12, PixelFormat::RGBA8 },
        { source.data(), 3, 3, 11, PixelFormat::RGBA8 },
        { source.data(), 3, 3, std::numeric_limits<std::ptrdiff_t>::max(),
          PixelFormat::RGBA8 },
        { source.data(), 3, 3, 12, static_cast<PixelFormat>(2) }
    };
    for(auto mode : { Interpolation::Nearest, Interpolation::Linear }) {
        for(const auto &input : invalid) {
            REQUIRE_THROWS_AS(Resize(input, destination.view(), mode),
                              std::invalid_argument);
            const ImageView badDestination{
                const_cast<std::uint8_t *>(input.data), input.width,
                input.height, input.pitch, input.format
            };
            REQUIRE_THROWS_AS(Resize(valid, badDestination, mode),
                              std::invalid_argument);
        }
    }
    REQUIRE_THROWS_AS(
        Resize(valid, destination.view(), static_cast<Interpolation>(4)),
        std::invalid_argument);
    auto wrong = destination.view();
    wrong.format = PixelFormat::Gray8;
    REQUIRE_THROWS_AS(Resize(valid, wrong, Interpolation::Linear),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(BoxBlur(valid, destination.view(), 0, 3),
                      std::invalid_argument);
    wrong = destination.view();
    wrong.width = 2;
    REQUIRE_THROWS_AS(BoxBlur(valid, wrong, 3, 3), std::invalid_argument);
    REQUIRE(destination.bytes == original);
    REQUIRE_THROWS_AS(ImageBuffer(0, 1, PixelFormat::Gray8),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(ImageBuffer(std::numeric_limits<int>::max(),
                                  std::numeric_limits<int>::max(),
                                  PixelFormat::RGBA8),
                      std::invalid_argument);
}
