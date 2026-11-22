#pragma once
#include "imageops/ImageOps.h"
#include <cstdint>
#include <vector>

namespace ImageOpsCases {
    struct Case {
        int operation, width, height, outputWidth, outputHeight, channels, mode,
            variant;
        std::uint32_t seed;
        std::uint64_t expected;
    };
    inline std::vector<std::uint8_t> pixels(int width, int height, int channels,
                                            std::size_t pitch,
                                            std::uint32_t seed) {
        const bool white = seed == 0xffffffffu;
        std::vector<std::uint8_t> result(pitch * height + 32, 0xa5);
        for(int y = 0; y < height; ++y)
            for(int x = 0; x < width * channels; ++x) {
                seed = seed * 1664525u + 1013904223u;
                result[16 + y * pitch + x] =
                    white ? 255 : static_cast<std::uint8_t>(seed >> 24);
            }
        return result;
    }
    inline std::uint64_t hash(const std::uint8_t *data, int width, int height,
                              int channels, std::size_t pitch) {
        std::uint64_t value = 14695981039346656037ull;
        for(int y = 0; y < height; ++y)
            for(int x = 0; x < width * channels; ++x) {
                value ^= data[y * pitch + x];
                value *= 1099511628211ull;
            }
        return value;
    }
    inline TVPImageOps::Quad from(const Case &c) {
        return { TVPImageOps::Point(0, 0), TVPImageOps::Point(c.width - 1.f, 0),
                 TVPImageOps::Point(c.width - 1.f, c.height - 1.f),
                 TVPImageOps::Point(0, c.height - 1.f) };
    }
    inline TVPImageOps::Quad to(const Case &c) {
        auto q = from(c);
        if(c.variant == 0)
            return q;
        if(c.variant == 1) {
            for(auto &p : q) {
                p.x += 2;
                p.y -= 1;
            }
            return q;
        }
        if(c.variant == 2)
            return { TVPImageOps::Point(0, 0), TVPImageOps::Point(0, 0),
                     TVPImageOps::Point(0, 0), TVPImageOps::Point(0, 0) };
        q[0].x -= 1.3f;
        q[0].y += .4f;
        q[1].x += .8f;
        q[1].y -= .7f;
        q[2].x += 1.1f;
        q[2].y -= 1.7f;
        q[3].x += .2f;
        q[3].y += 1.1f;
        return q;
    }
} // namespace ImageOpsCases
