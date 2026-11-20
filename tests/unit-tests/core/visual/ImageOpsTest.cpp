#include "ImageOpsGolden.h"
#include "imageops/ImageOps_internal.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <stdexcept>

using namespace TVPImageOps;
namespace {
    struct Pixels {
        int width, height, channels;
        std::size_t pitch;
        std::vector<std::uint8_t> bytes;
        Pixels(int w, int h, int cn, std::uint32_t seed = 0) :
            width(w), height(h), channels(cn),
            pitch(static_cast<std::size_t>(w) * cn + 17),
            bytes(ImageOpsCases::pixels(w, h, cn, pitch, seed)) {}
        ImageView view() {
            return { bytes.data() + 16, width, height,
                     static_cast<std::ptrdiff_t>(pitch),
                     channels == 1 ? PixelFormat::Gray8 : PixelFormat::RGBA8 };
        }
        std::uint64_t hash() {
            return ImageOpsCases::hash(view().data, width, height, channels,
                                       pitch);
        }
        void guards() {
            bool intact = true;
            for(std::size_t i = 0; i < bytes.size(); ++i) {
                const auto offset = i >= 16 ? i - 16 : pitch * height;
                if(i < 16 || offset >= pitch * height ||
                   offset % pitch >= static_cast<std::size_t>(width) * channels)
                    intact = intact && bytes[i] == 0xa5;
            }
            REQUIRE(intact);
        }
    };
    struct RestoreThreads {
        int count = GetThreadCount();
        ~RestoreThreads() { SetThreadCount(count); }
    };
} // namespace
TEST_CASE("ImageOps all algorithms retain frozen OpenCV 4.7 outputs",
          "[imageops][golden]") {
    RestoreThreads restore;
    for(int threads : { 1, 3 }) {
        SetThreadCount(threads);
        for(const auto &c : ImageOpsCases::golden) {
            CAPTURE(threads, c.operation, c.width, c.height, c.outputWidth,
                    c.outputHeight, c.channels, c.mode, c.variant, c.seed);
            Pixels src(c.width, c.height, c.channels, c.seed);
            const auto before = src.bytes;
            const auto mode = static_cast<Interpolation>(c.mode);
            if(c.operation <= 1) {
                Pixels dst(c.operation == 0 ? c.outputWidth : c.width,
                           c.operation == 0 ? c.outputHeight : c.height,
                           c.channels);
                if(c.operation == 0)
                    Resize(src.view(), dst.view(), mode);
                else
                    BoxBlur(src.view(), dst.view(), c.outputWidth,
                            c.outputHeight);
                CHECK(dst.hash() == c.expected);
                dst.guards();
                if(c.operation == 1) {
                    REQUIRE(src.bytes == before);
                    BoxBlur(src.view(), src.view(), c.outputWidth,
                            c.outputHeight);
                    CHECK(src.hash() == c.expected);
                }
            } else {
                const auto a = ImageOpsCases::from(c), b = ImageOpsCases::to(c);
                auto output = c.operation == 2
                    ? WarpAffine(src.view(), c.outputWidth, c.outputHeight, a,
                                 b, mode)
                    : WarpPerspective(src.view(), c.outputWidth, c.outputHeight,
                                      a, b, mode);
                CHECK(ImageOpsCases::hash(output.data(), output.width(),
                                          output.height(), c.channels,
                                          output.pitch()) == c.expected);
            }
            if(c.operation != 1)
                CHECK(src.bytes == before);
            src.guards();
        }
    }
}
TEST_CASE("ImageOps has explicit nearest pixels and independent alpha",
          "[imageops]") {
    const std::uint8_t input[] = { 1, 2, 3, 4 };
    std::uint8_t result[16]{};
    Resize({ input, 2, 2, 0, PixelFormat::Gray8 },
           { result, 4, 4, 0, PixelFormat::Gray8 }, Interpolation::Nearest);
    const std::uint8_t expected[] = { 1, 1, 2, 2, 1, 1, 2, 2,
                                      3, 3, 4, 4, 3, 3, 4, 4 };
    REQUIRE(
        std::equal(std::begin(result), std::end(result), std::begin(expected)));
    const std::uint8_t rgba[] = { 255, 0, 0, 0, 0, 0, 0, 255 };
    std::uint8_t blended[12]{};
    Resize({ rgba, 2, 1, 0, PixelFormat::RGBA8 },
           { blended, 3, 1, 0, PixelFormat::RGBA8 }, Interpolation::Linear);
    REQUIRE(blended[4] == 128);
    REQUIRE(blended[7] == 128);
    const std::uint8_t six[] = { 10, 20, 30, 40, 50, 60 };
    std::uint8_t expanded[34]{};
    Resize({ six, 6, 1, 0, PixelFormat::Gray8 },
           { expanded, 34, 1, 0, PixelFormat::Gray8 }, Interpolation::Nearest);
    REQUIRE(expanded[17] == 30);
    REQUIRE(expanded[18] == 40);
}
TEST_CASE("ImageOps identity and cropped views preserve guard bytes",
          "[imageops]") {
    for(int cn : { 1, 4 })
        for(auto mode : { Interpolation::Nearest, Interpolation::Area,
                          Interpolation::Linear, Interpolation::Cubic }) {
            Pixels src(19, 13, cn, 71), dst(19, 13, cn);
            Resize(src.view(), dst.view(), mode);
            REQUIRE(src.bytes == dst.bytes);
            const auto before = src.bytes;
            Resize(src.view(), src.view(), mode);
            REQUIRE(src.bytes == before);
            Pixels crop(9, 7, cn, 35), out(13, 11, cn), tight(5, 3, cn, 3),
                expected(7, 5, cn);
            auto sv = crop.view(), dv = out.view();
            sv.data += crop.pitch + 2 * cn;
            sv.width = 5;
            sv.height = 3;
            dv.data += out.pitch + 3 * cn;
            dv.width = 7;
            dv.height = 5;
            for(int y = 0; y < 3; ++y)
                std::copy_n(sv.data + y * sv.pitch, 5 * cn,
                            tight.view().data + y * tight.pitch);
            auto expectedWhole = out.bytes;
            Resize(tight.view(), expected.view(), mode);
            Resize(sv, dv, mode);
            for(int y = 0; y < 5; ++y)
                REQUIRE(std::equal(dv.data + y * dv.pitch,
                                   dv.data + y * dv.pitch + 7 * cn,
                                   expected.view().data + y * expected.pitch));
            for(int y = 0; y < 5; ++y)
                std::copy_n(expected.view().data + y * expected.pitch, 7 * cn,
                            expectedWhole.data() + 16 + (y + 1) * out.pitch +
                                3 * cn);
            REQUIRE(out.bytes == expectedWhole);
            out.guards();
            crop.guards();
        }
}
TEST_CASE("ImageOps transform matrices map float vertices and singular inputs",
          "[imageops]") {
    const Quad from{ Point(0, 0), Point(7, 0), Point(7, 5), Point(0, 5) };
    const Quad to{ Point(1.2f, -.4f), Point(8.3f, 1.1f), Point(7.2f, 6.4f),
                   Point(-.1f, 5.2f) };
    const auto matrix = PerspectiveTransform(from, to);
    for(int i = 0; i < 4; ++i) {
        const double d =
            matrix[6] * from[i].x + matrix[7] * from[i].y + matrix[8];
        REQUIRE(std::abs((matrix[0] * from[i].x + matrix[1] * from[i].y +
                          matrix[2]) /
                             d -
                         to[i].x) < 1e-5);
        REQUIRE(std::abs((matrix[3] * from[i].x + matrix[4] * from[i].y +
                          matrix[5]) /
                             d -
                         to[i].y) < 1e-5);
    }
    const Quad degenerate{ Point(0, 0), Point(0, 0), Point(0, 0), Point(0, 0) };
    const Matrix3 singular{ 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    REQUIRE(PerspectiveTransform(degenerate, to) == singular);
    Pixels source(7, 6, 4, 13);
    auto owned = WarpPerspective(source.view(), 7, 6, from, from,
                                 Interpolation::Nearest);
    const std::vector<std::uint8_t> snapshot(
        owned.data(), owned.data() + owned.pitch() * owned.height());
    auto moved = std::move(owned);
    for(int i = 0; i < 8; ++i)
        WarpAffine(source.view(), 9, 8, from, to, Interpolation::Cubic);
    REQUIRE(std::equal(snapshot.begin(), snapshot.end(), moved.data()));
}
TEST_CASE("ImageOps rejects invalid requests before writing", "[imageops]") {
    Pixels src(4, 3, 4, 1), dst(4, 3, 4);
    const auto before = dst.bytes;
    for(auto mode : { Interpolation::Nearest, Interpolation::Area,
                      Interpolation::Linear, Interpolation::Cubic }) {
        auto bad = src.view();
        bad.data = nullptr;
        REQUIRE_THROWS_AS(Resize(bad, dst.view(), mode), std::invalid_argument);
        bad = dst.view();
        bad.pitch = 1;
        REQUIRE_THROWS_AS(Resize(src.view(), bad, mode), std::invalid_argument);
        bad = dst.view();
        bad.width = 0;
        REQUIRE_THROWS_AS(Resize(src.view(), bad, mode), std::invalid_argument);
    }
    REQUIRE_THROWS_AS(
        Resize(src.view(), dst.view(), static_cast<Interpolation>(4)),
        std::invalid_argument);
    REQUIRE_THROWS_AS(BoxBlur(src.view(), dst.view(), 0, 3),
                      std::invalid_argument);
    auto bad = dst.view();
    bad.height = 2;
    REQUIRE_THROWS_AS(BoxBlur(src.view(), bad, 3, 3), std::invalid_argument);
    bad = src.view();
    bad.data = nullptr;
    REQUIRE_THROWS_AS(BoxBlur(bad, dst.view(), 3, 3), std::invalid_argument);
    const Quad q{ Point(0, 0), Point(3, 0), Point(3, 2), Point(0, 2) };
    auto nonfinite = q;
    nonfinite[0].x = std::numeric_limits<float>::infinity();
    REQUIRE_THROWS_AS(PerspectiveTransform(q, nonfinite),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(
        WarpAffine(src.view(), 3, 3, q, q, static_cast<Interpolation>(-1)),
        std::invalid_argument);
    REQUIRE_THROWS_AS(ImageBuffer(0, 1, PixelFormat::Gray8),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(SetThreadCount(-1), std::invalid_argument);
    REQUIRE(dst.bytes == before);
}
TEST_CASE(
    "ImageOps row scheduling joins exceptions and supports concurrent callers",
    "[imageops][threads]") {
    RestoreThreads restore;
    SetThreadCount(3);
    std::vector<int> visits(1000);
    detail::ForRows(1000, 1000000, [&](int a, int b) {
        for(int i = a; i < b; ++i)
            ++visits[i];
    });
    REQUIRE(std::all_of(visits.begin(), visits.end(),
                        [](int n) { return n == 1; }));
    REQUIRE_THROWS_AS(detail::ForRows(1000, 1000000,
                                      [](int a, int) {
                                          if(a)
                                              throw std::runtime_error(
                                                  "worker");
                                      }),
                      std::runtime_error);
    REQUIRE_THROWS_AS(detail::ForRows(1000, 1000000,
                                      [](int a, int) {
                                          if(!a)
                                              throw std::runtime_error(
                                                  "caller");
                                      }),
                      std::runtime_error);
    Pixels blurSource(601, 419, 4, 67), serialBlur(601, 419, 4),
        parallelBlur(601, 419, 4);
    SetThreadCount(1);
    BoxBlur(blurSource.view(), serialBlur.view(), 9, 9);
    SetThreadCount(3);
    BoxBlur(blurSource.view(), parallelBlur.view(), 9, 9);
    REQUIRE(serialBlur.bytes == parallelBlur.bytes);
    BoxBlur(blurSource.view(), blurSource.view(), 9, 9);
    REQUIRE(blurSource.bytes == parallelBlur.bytes);
    const auto run = [] {
        Pixels src(601, 419, 4, 67), dst(271, 263, 4);
        Resize(src.view(), dst.view(), Interpolation::Cubic);
        return dst.hash();
    };
    auto a = std::async(std::launch::async, run),
         b = std::async(std::launch::async, run);
    REQUIRE(a.get() == b.get());
}
