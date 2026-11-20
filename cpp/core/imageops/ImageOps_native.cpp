/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this
license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, 2017, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2014-2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without
modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright
notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote
products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is"
and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are
disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any
direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this
license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2014-2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without
modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright
notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote
products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is"
and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are
disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any
direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this
license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, 2018, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2014-2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without
modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright
notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote
products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is"
and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are
disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any
direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this
license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without
modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright
notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote
products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is"
and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are
disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any
direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

// Algorithms adapted from OpenCV 4.7.0, commit
// 725e440d278aca07d35a5e8963ef990572b07316. See LICENSES and UPSTREAM.md.
// Modified for borrowed Gray8/RGBA8 views and standard C++17; no OpenCV
// runtime.
#include "ImageOps_internal.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <future>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#endif

namespace TVPImageOps {
    namespace {
        std::atomic<int> threadCount{ 0 };
        int rounded(double v) {
            if(!std::isfinite(v))
                return 0;
            v = std::clamp(v, double(std::numeric_limits<int>::min()),
                           double(std::numeric_limits<int>::max()));
            const double lo = std::floor(v), fraction = v - lo;
            const int base = static_cast<int>(lo);
            return base + (fraction > .5 || (fraction == .5 && (base & 1)));
        }
        std::uint8_t byte(int v) {
            return static_cast<std::uint8_t>(std::clamp(v, 0, 255));
        }
        int clipped(int v, int n) { return std::clamp(v, 0, n - 1); }
        int shortCoord(int v) { return std::clamp(v, -32768, 32767); }
        void checkMode(Interpolation mode) {
            if(mode < Interpolation::Nearest || mode > Interpolation::Cubic)
                throw std::invalid_argument(
                    "ImageOps: unsupported interpolation");
        }
        void checkPoints(const Quad &points) {
            for(const auto &p : points)
                if(!std::isfinite(p.x) || !std::isfinite(p.y))
                    throw std::invalid_argument(
                        "ImageOps: nonfinite transform point");
        }
        std::array<float, 4> cubic(float t) {
            constexpr float a = -.75f;
            std::array<float, 4> w;
            w[0] = ((a * (t + 1) - 5 * a) * (t + 1) + 8 * a) * (t + 1) - 4 * a;
            w[1] = ((a + 2) * t - (a + 3)) * t * t + 1;
            w[2] = ((a + 2) * (1 - t) - (a + 3)) * (1 - t) * (1 - t) + 1;
            w[3] = 1.f - w[0] - w[1] - w[2];
            return w;
        }
        struct FilterAxis {
            std::vector<int> offset;
            std::vector<std::array<int, 4>> weights;
            FilterAxis(int from, int to, bool isCubic, bool vertical) :
                offset(to), weights(to) {
                const double scale = 1. / (double(to) / from);
                const double inv = double(to) / from;
                for(int i = 0; i < to; ++i) {
                    float t;
                    int origin;
                    if(isCubic) {
                        t = static_cast<float>((i + .5) * scale - .5);
                        origin = static_cast<int>(std::floor(t));
                        t -= origin;
                    } else {
                        origin = static_cast<int>(std::floor(i * scale));
                        t = static_cast<float>((i + 1) - (origin + 1) * inv);
                        t = t <= 0 ? 0.f : t - std::floor(t);
                        if(!vertical && (origin < 0 || origin >= from - 1)) {
                            origin = clipped(origin, from);
                            t = 0;
                        }
                    }
                    offset[i] = origin;
                    const auto w = isCubic
                        ? cubic(t)
                        : std::array<float, 4>{ 1.f - t, t, 0, 0 };
                    for(int j = 0; j < 4; ++j)
                        weights[i][j] = rounded(w[j] * 2048.f);
                }
            }
        };
        struct AreaWeight {
            int source;
            float weight;
        };
        using AreaAxis = std::vector<std::vector<AreaWeight>>;
        AreaAxis areaAxis(int from, int to) {
            AreaAxis result(to);
            const double scale = 1. / (double(to) / from);
            for(int i = 0; i < to; ++i) {
                const double begin = i * scale, end = begin + scale,
                             cell = std::min(scale, from - begin);
                int first = static_cast<int>(std::ceil(begin)),
                    last = static_cast<int>(std::floor(end));
                last = std::min(last, from - 1);
                first = std::min(first, last);
                if(first - begin > 1e-3)
                    result[i].push_back(
                        { first - 1,
                          static_cast<float>((first - begin) / cell) });
                for(int j = first; j < last; ++j)
                    result[i].push_back({ j, static_cast<float>(1. / cell) });
                if(end - last > 1e-3)
                    result[i].push_back(
                        { last,
                          static_cast<float>(
                              std::min({ end - last, 1., cell }) / cell) });
            }
            return result;
        }
        void resizeArea(ConstImageView src, ImageView dst) {
            const auto sp = detail::Step(src), dp = detail::Step(dst);
            const int cn = detail::Channels(src.format);
            const bool integer =
                src.width % dst.width == 0 && src.height % dst.height == 0;
            const int sx = src.width / dst.width, sy = src.height / dst.height;
            if(integer) {
                const float scale = 1.f / (static_cast<float>(sx) * sy);
                detail::ForRows(
                    dst.height,
                    static_cast<std::size_t>(dst.width) * dst.height,
                    [&](int begin, int end) {
                        for(int y = begin; y < end; ++y)
                            for(int x = 0; x < dst.width; ++x)
                                for(int c = 0; c < cn; ++c) {
                                    std::int64_t sum = 0;
                                    for(int ky = 0; ky < sy; ++ky)
                                        for(int kx = 0; kx < sx; ++kx)
                                            sum +=
                                                src.data
                                                    [(static_cast<std::size_t>(
                                                          y) *
                                                          sy +
                                                      ky) *
                                                         sp +
                                                     (static_cast<std::size_t>(
                                                          x) *
                                                          sx +
                                                      kx) *
                                                         cn +
                                                     c];
                                    dst.data[y * dp + x * cn + c] = byte(
                                        sx == 2 && sy == 2
                                            ? static_cast<int>((sum + 2) >> 2)
                                            : rounded(static_cast<float>(sum) *
                                                      scale));
                                }
                    });
                return;
            }
            const auto ax = areaAxis(src.width, dst.width),
                       ay = areaAxis(src.height, dst.height);
            detail::ForRows(
                dst.height, static_cast<std::size_t>(dst.width) * dst.height,
                [&](int begin, int end) {
                    for(int y = begin; y < end; ++y)
                        for(int x = 0; x < dst.width; ++x)
                            for(int c = 0; c < cn; ++c) {
                                float sum = 0;
                                for(const auto &wy : ay[y]) {
                                    float row = 0;
                                    for(const auto &wx : ax[x])
                                        row += src.data[wy.source * sp +
                                                        wx.source * cn + c] *
                                            wx.weight;
                                    sum += row * wy.weight;
                                }
                                dst.data[y * dp + x * cn + c] =
                                    byte(rounded(sum));
                            }
                });
        }
        // Partial pivot LU, preserving the operation order of the former
        // solver.
        template <std::size_t N>
        bool solve(std::array<std::array<double, N>, N> a,
                   std::array<double, N> &b) {
            for(std::size_t i = 0; i < N; ++i) {
                std::size_t pivot = i;
                for(std::size_t j = i + 1; j < N; ++j)
                    if(std::abs(a[j][i]) > std::abs(a[pivot][i]))
                        pivot = j;
                if(std::abs(a[pivot][i]) <
                   std::numeric_limits<double>::epsilon() * 100) {
                    b.fill(0);
                    return false;
                }
                if(pivot != i) {
                    for(std::size_t k = i; k < N; ++k)
                        std::swap(a[i][k], a[pivot][k]);
                    std::swap(b[i], b[pivot]);
                }
                const double reciprocal = -1 / a[i][i];
                for(std::size_t j = i + 1; j < N; ++j) {
                    const double multiplier = a[j][i] * reciprocal;
                    for(std::size_t k = i + 1; k < N; ++k)
                        a[j][k] += multiplier * a[i][k];
                    b[j] += multiplier * b[i];
                }
            }
            for(int i = static_cast<int>(N) - 1; i >= 0; --i) {
                double sum = b[i];
                for(std::size_t k = i + 1; k < N; ++k)
                    sum -= a[i][k] * b[k];
                b[i] = sum / a[i][i];
            }
            return true;
        }
        Matrix3 affine(const Quad &from, const Quad &to) {
            std::array<std::array<double, 6>, 6> a{};
            std::array<double, 6> b{};
            for(int i = 0; i < 3; ++i) {
                a[i * 2][0] = a[i * 2 + 1][3] = from[i].x;
                a[i * 2][1] = a[i * 2 + 1][4] = from[i].y;
                a[i * 2][2] = a[i * 2 + 1][5] = 1;
                b[i * 2] = to[i].x;
                b[i * 2 + 1] = to[i].y;
            }
            solve(a, b);
            return { b[0], b[1], b[2], b[3], b[4], b[5], 0, 0, 1 };
        }
        Matrix3 inverse(const Matrix3 &m) {
            Matrix3 r{ m[4] * m[8] - m[5] * m[7], m[2] * m[7] - m[1] * m[8],
                       m[1] * m[5] - m[2] * m[4], m[5] * m[6] - m[3] * m[8],
                       m[0] * m[8] - m[2] * m[6], m[2] * m[3] - m[0] * m[5],
                       m[3] * m[7] - m[4] * m[6], m[1] * m[6] - m[0] * m[7],
                       m[0] * m[4] - m[1] * m[3] };
            const double d = m[0] * r[0] + m[1] * r[3] + m[2] * r[6];
            const double scale = d != 0 ? 1. / d : 0;
            for(auto &v : r)
                v *= scale;
            return r;
        }
        // 32x32 subpixel phases, Q15 coefficients (including legacy sum
        // correction).
        using WarpTable = std::array<std::array<short, 16>, 1024>;
        WarpTable makeWarpTable() {
            WarpTable table{};
            constexpr int n = 4;
            for(int y = 0; y < 32; ++y)
                for(int x = 0; x < 32; ++x) {
                    const auto wx = cubic(x / 32.f), wy = cubic(y / 32.f);
                    auto &w = table[y * 32 + x];
                    int total = 0;
                    for(int j = 0; j < n; ++j)
                        for(int i = 0; i < n; ++i) {
                            w[j * n + i] = static_cast<short>(
                                std::clamp(rounded(wy[j] * wx[i] * 32768.f),
                                           -32768, 32767));
                            total += w[j * n + i];
                        }
                    if(total != 32768) {
                        int lo = 10, hi = 10;
                        for(int j = 2; j < 4; ++j)
                            for(int i = 2; i < 4; ++i) {
                                int k = j * 4 + i;
                                if(w[k] < w[lo])
                                    lo = k;
                                else if(w[k] > w[hi])
                                    hi = k;
                            }
                        const int k = total < 32768 ? hi : lo;
                        w[k] = static_cast<short>(w[k] - (total - 32768));
                    }
                }
            return table;
        }
#if defined(_MSC_VER)
#define TVP_IMAGEOPS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define TVP_IMAGEOPS_INLINE inline __attribute__((always_inline))
#else
#define TVP_IMAGEOPS_INLINE inline
#endif
        template <int cn, Interpolation mode>
        TVP_IMAGEOPS_INLINE void sample(ConstImageView src, std::size_t stride,
                                        std::uint8_t *out, int x, int y, int fx,
                                        int fy) {
            const auto valid = [&](int px, int py) {
                return px >= 0 && py >= 0 && px < src.width && py < src.height;
            };
            if(mode == Interpolation::Nearest) {
                if(valid(x, y))
                    std::memcpy(out, src.data + y * stride + x * cn, cn);
                else
                    std::memset(out, 0, cn);
                return;
            }
#if defined(__SSE2__) || defined(_M_X64)
            if constexpr(cn == 4 && mode == Interpolation::Linear) {
                if(x >= 0 && y >= 0 && x < src.width - 1 &&
                   y < src.height - 1) {
                    const auto *top = src.data + y * stride + x * 4;
                    const auto zero = _mm_setzero_si128();
                    auto a = _mm_unpacklo_epi8(
                        _mm_loadl_epi64(reinterpret_cast<const __m128i *>(top)),
                        zero);
                    auto b = _mm_unpacklo_epi8(
                        _mm_loadl_epi64(
                            reinterpret_cast<const __m128i *>(top + stride)),
                        zero);
                    a = _mm_unpacklo_epi16(a, _mm_srli_si128(a, 8));
                    b = _mm_unpacklo_epi16(b, _mm_srli_si128(b, 8));
                    const auto wa = _mm_set1_epi32(((fx * (32 - fy)) << 16) |
                                                   ((32 - fx) * (32 - fy)));
                    const auto wb =
                        _mm_set1_epi32(((fx * fy) << 16) | ((32 - fx) * fy));
                    auto value = _mm_add_epi32(_mm_madd_epi16(a, wa),
                                               _mm_madd_epi16(b, wb));
                    value = _mm_srli_epi32(
                        _mm_add_epi32(value, _mm_set1_epi32(512)), 10);
                    const auto packed =
                        _mm_packus_epi16(_mm_packs_epi32(value, zero), zero);
                    const auto pixel =
                        static_cast<std::uint32_t>(_mm_cvtsi128_si32(packed));
                    std::memcpy(out, &pixel, 4);
                    return;
                }
            }
#endif
            const std::uint8_t zero[4]{};
            const auto pointer = [&](int px, int py) {
                return valid(px, py) ? src.data + py * stride + px * cn : zero;
            };
            if(mode != Interpolation::Cubic) {
                const auto *p00 = pointer(x, y), *p01 = pointer(x + 1, y),
                           *p10 = pointer(x, y + 1),
                           *p11 = pointer(x + 1, y + 1);
                const int a = (32 - fx) * (32 - fy), b = fx * (32 - fy),
                          c = (32 - fx) * fy, d = fx * fy;
                for(int ch = 0; ch < cn; ++ch)
                    out[ch] = static_cast<std::uint8_t>(
                        (p00[ch] * a + p01[ch] * b + p10[ch] * c + p11[ch] * d +
                         512) >>
                        10);
                return;
            }
            static const auto table = makeWarpTable();
            const auto &w = table[fy * 32 + fx];
            int sums[4]{};
            for(int j = 0; j < 4; ++j)
                for(int i = 0; i < 4; ++i) {
                    const auto *pixel = pointer(x + i - 1, y + j - 1);
                    for(int ch = 0; ch < cn; ++ch)
                        sums[ch] += pixel[ch] * w[j * 4 + i];
                }
            for(int ch = 0; ch < cn; ++ch)
                out[ch] = byte((sums[ch] + 16384) >> 15);
        }
#undef TVP_IMAGEOPS_INLINE
        int coordinate(double value) {
            value = std::clamp(value, double(std::numeric_limits<int>::min()),
                               double(std::numeric_limits<int>::max()));
#if defined(__SSE2__) || defined(_M_X64)
            return _mm_cvtsd_si32(_mm_set_sd(value));
#else
            return static_cast<int>(std::lrint(value));
#endif
        }
        template <int cn, bool projective, Interpolation mode>
        void warpRows(ConstImageView src, ImageBuffer &dst, const Matrix3 &m,
                      const std::vector<int> &xdelta,
                      const std::vector<int> &ydelta) {
            const auto stride = detail::Step(src),
                       dp = static_cast<std::size_t>(dst.pitch());
            const int width = dst.width(), height = dst.height();
            detail::ForRows(
                height, static_cast<std::size_t>(width) * height,
                [&](int begin, int end) {
                    const int block =
                        std::min(1024 / std::min(16, height), width);
                    for(int y = begin; y < end; ++y) {
                        auto *output = dst.data() + y * dp;
                        const auto emit = [&](int x, int ix, int iy) {
                            int fx = 0, fy = 0;
                            if constexpr(mode != Interpolation::Nearest) {
                                fx = ix & 31;
                                fy = iy & 31;
                                ix >>= 5;
                                iy >>= 5;
                            }
                            sample<cn, mode>(src, stride, output + x * cn,
                                             shortCoord(ix), shortCoord(iy), fx,
                                             fy);
                        };
                        if constexpr(projective) {
                            for(int base = 0; base < width; base += block) {
                                const int count = std::min(block, width - base);
                                const double xx = m[0] * base + m[1] * y + m[2],
                                             yy = m[3] * base + m[4] * y + m[5],
                                             ww = m[6] * base + m[7] * y + m[8];
                                int x = 0;
#if defined(__SSE2__) || defined(_M_X64)
                                const auto zero = _mm_setzero_pd();
                                const auto lower = _mm_set1_pd(
                                               std::numeric_limits<int>::min()),
                                           upper = _mm_set1_pd(
                                               std::numeric_limits<int>::max());
                                const auto numerator = _mm_set1_pd(
                                    mode == Interpolation::Nearest ? 1. : 32.);
                                for(; x + 2 <= count; x += 2) {
                                    const auto local = _mm_set_pd(x + 1, x);
                                    auto w = _mm_add_pd(
                                        _mm_set1_pd(ww),
                                        _mm_mul_pd(_mm_set1_pd(m[6]), local));
                                    // Do not divide by zero, including
                                    // floating-point exception traps.
                                    const auto nonzero = _mm_cmpneq_pd(w, zero);
                                    w = _mm_or_pd(
                                        _mm_and_pd(nonzero, w),
                                        _mm_andnot_pd(nonzero,
                                                      _mm_set1_pd(1.)));
                                    const auto scale = _mm_and_pd(
                                        nonzero, _mm_div_pd(numerator, w));
                                    auto cx = _mm_mul_pd(
                                        _mm_add_pd(_mm_set1_pd(xx),
                                                   _mm_mul_pd(_mm_set1_pd(m[0]),
                                                              local)),
                                        scale);
                                    auto cy = _mm_mul_pd(
                                        _mm_add_pd(_mm_set1_pd(yy),
                                                   _mm_mul_pd(_mm_set1_pd(m[3]),
                                                              local)),
                                        scale);
                                    auto ix = _mm_cvtpd_epi32(_mm_min_pd(
                                        upper, _mm_max_pd(lower, cx)));
                                    auto iy = _mm_cvtpd_epi32(_mm_min_pd(
                                        upper, _mm_max_pd(lower, cy)));
                                    emit(base + x, _mm_cvtsi128_si32(ix),
                                         _mm_cvtsi128_si32(iy));
                                    emit(base + x + 1,
                                         _mm_cvtsi128_si32(
                                             _mm_srli_si128(ix, 4)),
                                         _mm_cvtsi128_si32(
                                             _mm_srli_si128(iy, 4)));
                                }
#endif
                                for(; x < count; ++x) {
                                    const double w = ww + m[6] * x,
                                                 scale = w != 0
                                        ? (mode == Interpolation::Nearest
                                               ? 1.
                                               : 32.) /
                                            w
                                        : 0;
                                    emit(base + x,
                                         coordinate((xx + m[0] * x) * scale),
                                         coordinate((yy + m[3] * x) * scale));
                                }
                            }
                        } else {
                            constexpr int delta =
                                mode == Interpolation::Nearest ? 512 : 16;
                            const auto xb =
                                static_cast<std::int64_t>(
                                    coordinate((m[1] * y + m[2]) * 1024)) +
                                delta;
                            const auto yb =
                                static_cast<std::int64_t>(
                                    coordinate((m[4] * y + m[5]) * 1024)) +
                                delta;
                            constexpr int shift =
                                mode == Interpolation::Nearest ? 10 : 5;
                            for(int x = 0; x < width; ++x)
                                emit(
                                    x,
                                    static_cast<int>((xb + xdelta[x]) >> shift),
                                    static_cast<int>((yb + ydelta[x]) >>
                                                     shift));
                        }
                    }
                });
        }
        template <int cn, bool projective>
        void dispatchWarp(ConstImageView src, ImageBuffer &dst,
                          const Matrix3 &m, const std::vector<int> &xd,
                          const std::vector<int> &yd, Interpolation mode) {
            if(mode == Interpolation::Nearest)
                warpRows<cn, projective, Interpolation::Nearest>(src, dst, m,
                                                                 xd, yd);
            else if(mode == Interpolation::Cubic)
                warpRows<cn, projective, Interpolation::Cubic>(src, dst, m, xd,
                                                               yd);
            else
                warpRows<cn, projective, Interpolation::Linear>(src, dst, m, xd,
                                                                yd);
        }
        ImageBuffer warp(ConstImageView src, int width, int height,
                         const Quad &from, const Quad &to, Interpolation mode,
                         bool projective) {
            detail::Step(src);
            checkMode(mode);
            checkPoints(from);
            checkPoints(to);
            ImageBuffer dst(width, height, src.format);
            auto m = projective ? inverse(PerspectiveTransform(from, to))
                                : affine(from, to);
            if(!projective) {
                double d = m[0] * m[4] - m[1] * m[3];
                d = d != 0 ? 1. / d : 0;
                const double a = m[4] * d, b = m[0] * d;
                m[0] = a;
                m[1] *= -d;
                m[3] *= -d;
                m[4] = b;
                const double tx = -m[0] * m[2] - m[1] * m[5],
                             ty = -m[3] * m[2] - m[4] * m[5];
                m[2] = tx;
                m[5] = ty;
            }
            const int cn = detail::Channels(src.format);
            std::vector<int> xdelta(projective ? 0 : width),
                ydelta(projective ? 0 : width);
            if(!projective)
                for(int x = 0; x < width; ++x) {
                    xdelta[x] = coordinate(m[0] * x * 1024);
                    ydelta[x] = coordinate(m[3] * x * 1024);
                }
            if(projective) {
                if(cn == 1)
                    dispatchWarp<1, true>(src, dst, m, xdelta, ydelta, mode);
                else
                    dispatchWarp<4, true>(src, dst, m, xdelta, ydelta, mode);
            } else {
                if(cn == 1)
                    dispatchWarp<1, false>(src, dst, m, xdelta, ydelta, mode);
                else
                    dispatchWarp<4, false>(src, dst, m, xdelta, ydelta, mode);
            }
            return dst;
        }
        int reflect(std::int64_t i, int n) {
            if(n == 1)
                return 0;
            const std::int64_t period = 2LL * (n - 1);
            i %= period;
            if(i < 0)
                i += period;
            return static_cast<int>(i < n ? i : period - i);
        }
    } // namespace
    void SetThreadCount(int count) {
        if(count < 0)
            throw std::invalid_argument("ImageOps: negative thread count");
        threadCount.store(count, std::memory_order_relaxed);
    }
    int GetThreadCount() {
        const int n = threadCount.load(std::memory_order_relaxed);
        return n ? n
                 : static_cast<int>(std::min(
                       8u, std::max(1u, std::thread::hardware_concurrency())));
    }
    void detail::ForRows(int height, std::size_t pixels,
                         const std::function<void(int, int)> &task) {
        const int workers = static_cast<int>(
            std::min({ static_cast<std::size_t>(GetThreadCount()),
                       static_cast<std::size_t>(height),
                       std::max(std::size_t(1), pixels / 65536) }));
        if(workers <= 1) {
            task(0, height);
            return;
        }
        std::vector<std::future<void>> jobs;
        for(int i = 1; i < workers; ++i)
            jobs.emplace_back(std::async(std::launch::async, [&, i] {
                task(static_cast<int>(static_cast<std::int64_t>(height) * i /
                                      workers),
                     static_cast<int>(static_cast<std::int64_t>(height) *
                                      (i + 1) / workers));
            }));
        // futures join during unwinding too; borrowed pixels never outlive this
        // call.
        task(0, height / workers);
        for(auto &job : jobs)
            job.get();
    }
    void detail::ResizeFiltered(ConstImageView src, ImageView dst,
                                Interpolation mode) {
        const auto sp = Step(src), dp = Step(dst);
        if(src.width == dst.width && src.height == dst.height) {
            if(src.data != dst.data)
                for(int y = 0; y < src.height; ++y)
                    std::memcpy(dst.data + y * dp, src.data + y * sp,
                                RowBytes(src.width, src.format));
            return;
        }
        if(mode == Interpolation::Area && src.width >= dst.width &&
           src.height >= dst.height) {
            resizeArea(src, dst);
            return;
        }
        const bool isCubic = mode == Interpolation::Cubic;
        const int cn = Channels(src.format), n = isCubic ? 4 : 2;
        const FilterAxis ax(src.width, dst.width, isCubic, false),
            ay(src.height, dst.height, isCubic, true);
        ForRows(
            dst.height, static_cast<std::size_t>(dst.width) * dst.height,
            [&](int begin, int end) {
                std::vector<std::vector<int>> rows(
                    n,
                    std::vector<int>(static_cast<std::size_t>(dst.width) * cn));
                std::vector<int> cached(n, -1);
                for(int y = begin; y < end; ++y) {
                    for(int j = 0; j < n; ++j) {
                        const int sy = clipped(
                            ay.offset[y] + j - (isCubic ? 1 : 0), src.height);
                        if(cached[j] == sy)
                            continue;
                        auto &row = rows[j];
                        const auto *in = src.data + sy * sp;
                        for(int x = 0; x < dst.width; ++x)
                            for(int c = 0; c < cn; ++c) {
                                int sum = 0;
                                for(int k = 0; k < n; ++k)
                                    sum += in[clipped(ax.offset[x] + k -
                                                          (isCubic ? 1 : 0),
                                                      src.width) *
                                                  cn +
                                              c] *
                                        ax.weights[x][k];
                                row[x * cn + c] = sum;
                            }
                        cached[j] = sy;
                    }
                    auto *out = dst.data + y * dp;
                    for(std::size_t x = 0; x < rows[0].size(); ++x) {
                        if(!isCubic)
                            out[x] =
                                byte(((((rows[0][x] >> 4) * ay.weights[y][0]) >>
                                       16) +
                                      (((rows[1][x] >> 4) * ay.weights[y][1]) >>
                                       16) +
                                      2) >>
                                     2);
                        else {
                            if(x < rows[0].size() / 8 * 8) {
                                const auto &w = ay.weights[y];
                                const float scale = 1.f / (2048 * 2048);
                                float sum = static_cast<float>(rows[3][x]) *
                                    (w[3] * scale);
                                for(int j = 2; j >= 0; --j)
                                    sum = static_cast<float>(rows[j][x]) *
                                            (w[j] * scale) +
                                        sum;
                                out[x] = byte(rounded(sum));
                            } else {
                                int sum = 0;
                                for(int j = 0; j < 4; ++j)
                                    sum += rows[j][x] * ay.weights[y][j];
                                out[x] = byte((sum + (1 << 21)) >> 22);
                            }
                        }
                    }
                }
            });
    }
    namespace {
        // 16-bit sums suffice for kernels with at most 256 pixels. Keep an
        // expanded border row so the running sum reads consecutive memory.
        template <int cn>
        void smallBoxBlur(ConstImageView src, ImageView dst, int kw, int kh) {
            const auto sp = detail::Step(src), dp = detail::Step(dst);
            const std::size_t width = static_cast<std::size_t>(src.width) * cn;
            const int area = kw * kh;
            const double quotient = (1 << 23) / static_cast<double>(area);
            int ds = static_cast<int>(std::floor(quotient)), dd = area / 2;
            if(quotient - ds < .5)
                ++dd;
            else
                ++ds;
            std::vector<std::uint16_t> rows(width * src.height);
            std::vector<int> border(src.width + kw);
            for(std::size_t x = 0; x < border.size(); ++x)
                border[x] =
                    reflect(static_cast<std::int64_t>(x) - kw / 2, src.width);
            detail::ForRows(
                src.height, static_cast<std::size_t>(src.width) * src.height,
                [&](int first, int last) {
                    std::vector<std::uint8_t> padded(border.size() * cn);
                    for(int y = first; y < last; ++y) {
                        const auto *input = src.data + y * sp;
                        for(std::size_t x = 0; x < border.size(); ++x)
                            std::memcpy(padded.data() + x * cn,
                                        input + border[x] * cn, cn);
                        auto *out = rows.data() + y * width;
#if defined(__SSE2__) || defined(_M_X64)
                        if constexpr(cn == 4) {
                            const auto load = [](const std::uint8_t *pixel) {
                                std::uint32_t value;
                                std::memcpy(&value, pixel, 4);
                                return _mm_unpacklo_epi8(
                                    _mm_cvtsi32_si128(static_cast<int>(value)),
                                    _mm_setzero_si128());
                            };
                            auto sum = _mm_setzero_si128();
                            for(int k = 0; k < kw; ++k)
                                sum = _mm_add_epi16(
                                    sum, load(padded.data() + k * 4));
                            for(int x = 0; x < src.width; ++x) {
                                _mm_storel_epi64(
                                    reinterpret_cast<__m128i *>(out + x * 4),
                                    sum);
                                sum = _mm_add_epi16(
                                    _mm_sub_epi16(sum,
                                                  load(padded.data() + x * 4)),
                                    load(padded.data() + (x + kw) * 4));
                            }
                        } else
#endif
                        {
                            int sum[cn]{};
                            for(int k = 0; k < kw; ++k)
                                for(int c = 0; c < cn; ++c)
                                    sum[c] += padded[k * cn + c];
                            for(int x = 0; x < src.width; ++x)
                                for(int c = 0; c < cn; ++c) {
                                    out[x * cn + c] =
                                        static_cast<std::uint16_t>(sum[c]);
                                    sum[c] += padded[(x + kw) * cn + c] -
                                        padded[x * cn + c];
                                }
                        }
                    }
                });
            detail::ForRows(
                src.height, static_cast<std::size_t>(src.width) * src.height,
                [&](int first, int last) {
                    std::vector<std::uint16_t> sum(width, 0);
                    for(int k = 0; k < kh; ++k) {
                        const auto *row = rows.data() +
                            reflect(first + k - kh / 2, src.height) * width;
                        for(std::size_t x = 0; x < width; ++x)
                            sum[x] =
                                static_cast<std::uint16_t>(sum[x] + row[x]);
                    }
                    for(int y = first; y < last; ++y) {
                        auto *out = dst.data + y * dp;
                        const auto *leaving = rows.data() +
                            reflect(static_cast<std::int64_t>(y) - kh / 2,
                                    src.height) *
                                width;
                        const auto *entering = rows.data() +
                            reflect(static_cast<std::int64_t>(y) + kh - kh / 2,
                                    src.height) *
                                width;
                        std::size_t x = 0;
#if defined(__SSE2__) || defined(_M_X64)
                        const auto multiplier = _mm_set1_epi32(ds),
                                   delta =
                                       _mm_set1_epi16(static_cast<short>(dd)),
                                   zero = _mm_setzero_si128();
                        const auto multiply = [&](__m128i v) {
                            const auto even = _mm_mul_epu32(v, multiplier);
                            const auto odd =
                                _mm_mul_epu32(_mm_srli_si128(v, 4), multiplier);
                            return _mm_unpacklo_epi32(
                                _mm_shuffle_epi32(even,
                                                  _MM_SHUFFLE(2, 0, 2, 0)),
                                _mm_shuffle_epi32(odd,
                                                  _MM_SHUFFLE(2, 0, 2, 0)));
                        };
                        for(; x + 8 <= width; x += 8) {
                            const auto v = _mm_loadu_si128(
                                reinterpret_cast<const __m128i *>(sum.data() +
                                                                  x));
                            const auto adjusted = _mm_add_epi16(v, delta);
                            const auto lo = _mm_srli_epi32(
                                multiply(_mm_unpacklo_epi16(adjusted, zero)),
                                23);
                            const auto hi = _mm_srli_epi32(
                                multiply(_mm_unpackhi_epi16(adjusted, zero)),
                                23);
                            _mm_storel_epi64(
                                reinterpret_cast<__m128i *>(out + x),
                                _mm_packus_epi16(_mm_packs_epi32(lo, hi),
                                                 zero));
                            const auto next = _mm_add_epi16(
                                _mm_sub_epi16(
                                    v,
                                    _mm_loadu_si128(
                                        reinterpret_cast<const __m128i *>(
                                            leaving + x))),
                                _mm_loadu_si128(
                                    reinterpret_cast<const __m128i *>(entering +
                                                                      x)));
                            _mm_storeu_si128(
                                reinterpret_cast<__m128i *>(sum.data() + x),
                                next);
                        }
#endif
                        for(; x < width; ++x) {
                            out[x] = static_cast<std::uint8_t>(
                                (static_cast<std::uint32_t>(sum[x]) + dd) *
                                    ds >>
                                23);
                            sum[x] = static_cast<std::uint16_t>(
                                sum[x] - leaving[x] + entering[x]);
                        }
                    }
                });
        }
        template <int Channels, typename Sum>
        void boxBlur(ConstImageView src, ImageView dst, int kw, int kh) {
            const auto sp = detail::Step(src), dp = detail::Step(dst);
            constexpr int cn = Channels;
            const auto width = detail::RowBytes(src.width, src.format);
            // Complete horizontal pass precedes destination writes, including
            // in-place views.
            std::vector<Sum> rows(width * static_cast<std::size_t>(src.height));
            std::vector<int> leave(src.width), enter(src.width);
            for(int x = 0; x < src.width; ++x) {
                leave[x] =
                    reflect(static_cast<std::int64_t>(x) - kw / 2, src.width) *
                    cn;
                enter[x] = reflect(static_cast<std::int64_t>(x) + kw - kw / 2,
                                   src.width) *
                    cn;
            }
            detail::ForRows(src.height,
                            static_cast<std::size_t>(src.width) * src.height,
                            [&](int first, int last) {
                                for(int y = first; y < last; ++y) {
                                    const auto *input = src.data + y * sp;
                                    auto *output = rows.data() + y * width;
                                    Sum sum[Channels]{};
                                    for(std::int64_t k = 0; k < kw; ++k) {
                                        const auto *pixel = input +
                                            reflect(k - kw / 2, src.width) * cn;
                                        for(int c = 0; c < cn; ++c)
                                            sum[c] += pixel[c];
                                    }
                                    for(int x = 0; x < src.width; ++x)
                                        for(int c = 0; c < cn; ++c) {
                                            output[x * cn + c] = sum[c];
                                            sum[c] -= input[leave[x] + c];
                                            sum[c] += input[enter[x] + c];
                                        }
                                }
                            });
            const double area = static_cast<double>(kw) * kh, scale = 1. / area;
            int divScale = 1, divDelta = 0;
            if(area <= 256 && area != 1) {
                const double quotient = (1 << 23) / area;
                divScale = static_cast<int>(std::floor(quotient));
                divDelta = static_cast<int>(area) / 2;
                if(quotient - divScale < .5)
                    ++divDelta;
                else
                    ++divScale;
            }
            detail::ForRows(
                src.height, static_cast<std::size_t>(src.width) * src.height,
                [&](int first, int last) {
                    std::vector<Sum> sum(width, 0);
                    for(std::int64_t k = 0; k < kh; ++k) {
                        const auto *row = rows.data() +
                            reflect(first + k - kh / 2, src.height) * width;
                        for(std::size_t x = 0; x < width; ++x)
                            sum[x] += row[x];
                    }
                    for(int y = first; y < last; ++y) {
                        auto *out = dst.data + y * dp;
                        if(area == 1)
                            for(std::size_t x = 0; x < width; ++x)
                                out[x] = static_cast<std::uint8_t>(sum[x]);
                        else if(area <= 256)
                            for(std::size_t x = 0; x < width; ++x)
                                out[x] = static_cast<std::uint8_t>(
                                    ((static_cast<std::uint32_t>(sum[x]) +
                                      divDelta) *
                                     static_cast<std::uint32_t>(divScale)) >>
                                    23);
                        else
                            for(std::size_t x = 0; x < width; ++x)
                                out[x] = byte(rounded(sum[x] * scale));
                        const auto *leaving = rows.data() +
                            reflect(static_cast<std::int64_t>(y) - kh / 2,
                                    src.height) *
                                width;
                        const auto *entering = rows.data() +
                            reflect(static_cast<std::int64_t>(y) + kh - kh / 2,
                                    src.height) *
                                width;
                        for(std::size_t x = 0; x < width; ++x) {
                            sum[x] -= leaving[x];
                            sum[x] += entering[x];
                        }
                    }
                });
        }
    } // namespace
    void BoxBlur(ConstImageView src, ImageView dst, int kw, int kh) {
        if(src.width != dst.width || src.height != dst.height ||
           src.format != dst.format || kw <= 0 || kh <= 0)
            throw std::invalid_argument(
                "ImageOps: invalid box blur dimensions");
        detail::Step(src);
        detail::Step(dst);
        if(static_cast<double>(kw) * kh >
           static_cast<double>(std::numeric_limits<std::uint64_t>::max() / 255))
            throw std::invalid_argument("ImageOps: box blur kernel too large");
        if(static_cast<double>(kw) * kh > 1 &&
           static_cast<double>(kw) * kh <= 256) {
            if(src.format == PixelFormat::Gray8)
                smallBoxBlur<1>(src, dst, kw, kh);
            else
                smallBoxBlur<4>(src, dst, kw, kh);
            return;
        }
        if(static_cast<double>(kw) * kh <= (1 << 23)) {
            if(src.format == PixelFormat::Gray8)
                boxBlur<1, std::uint32_t>(src, dst, kw, kh);
            else
                boxBlur<4, std::uint32_t>(src, dst, kw, kh);
        } else {
            if(src.format == PixelFormat::Gray8)
                boxBlur<1, std::uint64_t>(src, dst, kw, kh);
            else
                boxBlur<4, std::uint64_t>(src, dst, kw, kh);
        }
    }

    Matrix3 PerspectiveTransform(const Quad &from, const Quad &to) {
        checkPoints(from);
        checkPoints(to);
        std::array<std::array<double, 8>, 8> a{};
        std::array<double, 8> b{};
        for(int i = 0; i < 4; ++i) {
            a[i][0] = a[i + 4][3] = from[i].x;
            a[i][1] = a[i + 4][4] = from[i].y;
            a[i][2] = a[i + 4][5] = 1;
            // Products intentionally evaluated in float before promotion.
            a[i][6] = -from[i].x * to[i].x;
            a[i][7] = -from[i].y * to[i].x;
            a[i + 4][6] = -from[i].x * to[i].y;
            a[i + 4][7] = -from[i].y * to[i].y;
            b[i] = to[i].x;
            b[i + 4] = to[i].y;
        }
        solve(a, b);
        return { b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], 1 };
    }
    ImageBuffer WarpAffine(ConstImageView src, int width, int height,
                           const Quad &from, const Quad &to,
                           Interpolation mode) {
        return warp(src, width, height, from, to, mode, false);
    }
    ImageBuffer WarpPerspective(ConstImageView src, int width, int height,
                                const Quad &from, const Quad &to,
                                Interpolation mode) {
        return warp(src, width, height, from, to, mode, true);
    }
} // namespace TVPImageOps
