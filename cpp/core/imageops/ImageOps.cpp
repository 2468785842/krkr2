#include "ImageOps_internal.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#if defined(__SSE2__) || defined(_M_X64) ||                                    \
    (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define TVP_IMAGEOPS_SSE2 1
#endif

namespace TVPImageOps {
    namespace detail {
        int Channels(PixelFormat format) {
            switch(format) {
                case PixelFormat::Gray8:
                    return 1;
                case PixelFormat::RGBA8:
                    return 4;
            }
            throw std::invalid_argument("ImageOps: unsupported pixel format");
        }

        std::size_t RowBytes(int width, PixelFormat format) {
            if(width <= 0 ||
               static_cast<std::size_t>(width) >
                   static_cast<std::size_t>(
                       std::numeric_limits<std::ptrdiff_t>::max()) /
                       Channels(format))
                throw std::invalid_argument("ImageOps: invalid image width");
            return static_cast<std::size_t>(width) * Channels(format);
        }

        std::size_t Step(ConstImageView image) {
            const auto row = RowBytes(image.width, image.format);
            if(!image.data || image.height <= 0 || image.pitch < 0)
                throw std::invalid_argument("ImageOps: invalid image view");
            const auto stride =
                image.pitch ? static_cast<std::size_t>(image.pitch) : row;
            if(stride < row ||
               static_cast<std::size_t>(image.height - 1) >
                   (static_cast<std::size_t>(
                        std::numeric_limits<std::ptrdiff_t>::max()) -
                    row) /
                       stride)
                throw std::invalid_argument("ImageOps: invalid image pitch");
            return stride;
        }

    } // namespace detail

    namespace {
        template <int Channels>
        void nearestRows(const std::uint8_t *source, std::uint8_t *destination,
                         std::size_t sourcePitch, std::size_t destinationPitch,
                         int width, int sourceHeight, double inverseY,
                         const int *columns, int first, int last) {
            for(int y = first; y < last; ++y) {
                const int row =
                    std::min(static_cast<int>(y * inverseY), sourceHeight - 1);
                const auto *input = source + row * sourcePitch;
                auto *output = destination + y * destinationPitch;
                if constexpr(Channels == 1) {
                    for(int x = 0; x < width; ++x)
                        output[x] = input[columns[x]];
                } else {
                    for(int x = 0; x < width; ++x) {
                        // 小块 memcpy 支持未对齐的 ROI/pitch。
                        std::uint32_t pixel;
                        std::memcpy(
                            &pixel,
                            input + static_cast<std::size_t>(columns[x]) * 4,
                            sizeof(pixel));
                        std::memcpy(output + static_cast<std::size_t>(x) * 4,
                                    &pixel, sizeof(pixel));
                    }
                }
            }
        }

        void resizeNearest(ConstImageView source, ImageView destination) {
            const auto sourcePitch = detail::Step(source);
            const auto destinationPitch = detail::Step(destination);
            const int channels = detail::Channels(source.format);
            if(source.width == destination.width &&
               source.height == destination.height) {
                if(source.data == destination.data)
                    return;
                const auto bytes =
                    detail::RowBytes(source.width, source.format);
                for(int y = 0; y < source.height; ++y)
                    std::memcpy(destination.data + y * destinationPitch,
                                source.data + y * sourcePitch, bytes);
                return;
            }

            // 保持旧 INTER_NEAREST 的两次除法及 double 舍入，不能改成整数比值。
            // 例如 6 -> 34 的输出位置 17 仍采样源位置 2，而不是整数比值的 3。
            const double inverseX =
                1.0 / (static_cast<double>(destination.width) / source.width);
            const double inverseY =
                1.0 / (static_cast<double>(destination.height) / source.height);
            std::vector<int> columns(destination.width);
            for(int x = 0; x < destination.width; ++x)
                columns[x] =
                    std::min(static_cast<int>(x * inverseX), source.width - 1);

            detail::ForRows(
                destination.height,
                static_cast<std::size_t>(destination.width) *
                    destination.height,
                [=, indices = columns.data()](int first, int last) {
                    if(channels == 1)
                        nearestRows<1>(source.data, destination.data,
                                       sourcePitch, destinationPitch,
                                       destination.width, source.height,
                                       inverseY, indices, first, last);
                    else
                        nearestRows<4>(source.data, destination.data,
                                       sourcePitch, destinationPitch,
                                       destination.width, source.height,
                                       inverseY, indices, first, last);
                });
        }
    } // namespace

    namespace {
        // OpenCV 4.7.0 的 cvRound：四舍六入五取偶（与 FP 环境舍入模式无关，
        // 便于确定性复现）。本模块系数范围有限，减法精确，不会丢精度。
        int roundEven(float value) {
            const float lower = std::floor(value);
            const float diff = value - lower;
            const int base = static_cast<int>(lower);
            if(diff > 0.5f)
                return base + 1;
            if(diff < 0.5f)
                return base;
            return base + (base & 1);
        }

        short satShort(float value) {
            int v = roundEven(value);
            if(v > 32767)
                v = 32767;
            if(v < -32768)
                v = -32768;
            return static_cast<short>(v);
        }

        std::uint8_t satUchar(std::int32_t value) {
            if(value < 0)
                return 0;
            if(value > 255)
                return 255;
            return static_cast<std::uint8_t>(value);
        }

        int clipIndex(int value, int size) {
            return value < 0 ? 0 : (value < size ? value : size - 1);
        }

        // 复刻 OpenCV 4.7.0 resize.cpp 的 8 位线性系数生成：坐标映射为
        // double 计算后转 float，系数按 cvRound 变换为 Q11 定点数。垂直方向
        // 不钳制系数（fy 可为负，对应行复制的外推），行索引另行钳制。
        struct LinearPlan {
            std::vector<int> offsets;
            std::vector<short> coefficients;

            LinearPlan(int sourceSize, int destinationSize, int channels,
                       bool vertical) {
                const double scale =
                    static_cast<double>(sourceSize) / destinationSize;
                offsets.resize(static_cast<std::size_t>(destinationSize) *
                               (vertical ? 1 : channels));
                coefficients.resize(
                    static_cast<std::size_t>(vertical ? 2 : channels * 2) *
                    destinationSize);
                for(int out = 0; out < destinationSize; ++out) {
                    float fraction =
                        static_cast<float>((out + 0.5) * scale - 0.5);
                    int base = static_cast<int>(std::floor(fraction));
                    fraction -= base;
                    if(!vertical) {
                        // 水平方向按 OpenCV 的 xmin/xmax 边界钳制系数。
                        if(base < 0)
                            fraction = 0.f, base = 0;
                        if(base >= sourceSize - 1)
                            fraction = 0.f, base = sourceSize - 1;
                    }
                    const float weights[2] = { 1.f - fraction, fraction };
                    if(vertical) {
                        offsets[out] = base;
                        for(int k = 0; k < 2; ++k)
                            coefficients[static_cast<std::size_t>(out) * 2 +
                                         k] = satShort(weights[k] * 2048.f);
                    } else {
                        for(int ch = 0; ch < channels; ++ch)
                            offsets[static_cast<std::size_t>(out) * channels +
                                    ch] = base * channels + ch;
                        for(int ch = 0; ch < channels; ++ch)
                            for(int k = 0; k < 2; ++k)
                                coefficients[(static_cast<std::size_t>(out) *
                                                  channels +
                                              ch) *
                                                 2 +
                                             k] = satShort(weights[k] * 2048.f);
                    }
                }
            }
        };

        // 行缓存保存水平 Q11 和右移 4 位的结果，范围 0..32640，恰可用
        // int16 表示。垂直合成原本就先右移，因此不会提前丢失有效精度。
        void horizontalLinear(const std::uint8_t *source, short *output,
                              int width, int sourceWidth, int channels,
                              const LinearPlan &plan) {
            const int rowLanes = sourceWidth * channels;
            int i = 0;
#ifdef TVP_IMAGEOPS_SSE2
            if(channels == 4) {
                const auto zero = _mm_setzero_si128();
                for(; i < width; i += 4) {
                    const int sx = plan.offsets[i];
                    const int next = sx + 4 < rowLanes ? sx + 4 : sx;
                    std::int32_t left, right;
                    std::memcpy(&left, source + sx, 4);
                    std::memcpy(&right, source + next, 4);
                    const auto a =
                        _mm_unpacklo_epi8(_mm_cvtsi32_si128(left), zero);
                    const auto b =
                        _mm_unpacklo_epi8(_mm_cvtsi32_si128(right), zero);
                    const auto weights = _mm_set1_epi32(
                        static_cast<int>(static_cast<unsigned short>(
                            plan.coefficients[i * 2])) |
                        (static_cast<int>(plan.coefficients[i * 2 + 1]) << 16));
                    const auto sum =
                        _mm_madd_epi16(_mm_unpacklo_epi16(a, b), weights);
                    const auto packed =
                        _mm_packs_epi32(_mm_srli_epi32(sum, 4), zero);
                    _mm_storel_epi64(reinterpret_cast<__m128i *>(output + i),
                                     packed);
                }
            }
#endif
            for(; i < width; ++i) {
                const int sx = plan.offsets[i];
                const int next = sx + channels < rowLanes ? sx + channels : sx;
                output[i] = static_cast<short>(
                    (source[sx] * plan.coefficients[i * 2] +
                     source[next] * plan.coefficients[i * 2 + 1]) >>
                    4);
            }
        }

        void verticalLinear(const short *top, const short *bottom,
                            std::uint8_t *output, int width, short b0,
                            short b1) {
            int i = 0;
#ifdef TVP_IMAGEOPS_SSE2
            const auto beta0 = _mm_set1_epi16(b0), beta1 = _mm_set1_epi16(b1);
            const auto rounding = _mm_set1_epi16(2);
            for(; i <= width - 8; i += 8) {
                const auto a =
                    _mm_loadu_si128(reinterpret_cast<const __m128i *>(top + i));
                const auto b = _mm_loadu_si128(
                    reinterpret_cast<const __m128i *>(bottom + i));
                // signed mul_hi 与标量算术右移保持相同的负权重舍入。
                const auto sum = _mm_add_epi16(_mm_mulhi_epi16(a, beta0),
                                               _mm_mulhi_epi16(b, beta1));
                const auto result =
                    _mm_srai_epi16(_mm_add_epi16(sum, rounding), 2);
                _mm_storel_epi64(reinterpret_cast<__m128i *>(output + i),
                                 _mm_packus_epi16(result, result));
            }
#endif
            for(; i < width; ++i) {
                const int sum =
                    ((top[i] * b0) >> 16) + ((bottom[i] * b1) >> 16);
                output[i] = satUchar((sum + 2) >> 2);
            }
        }

        void linearRows(const std::uint8_t *source, std::uint8_t *destination,
                        std::size_t sourcePitch, std::size_t destinationPitch,
                        int width, int sourceWidth, int sourceHeight,
                        int channels, const LinearPlan &planX,
                        const LinearPlan &planY, int first, int last) {
            // 灰度缩小时没有足够的重复源行抵消缓存和分离两趟循环的成本。
            // 保留原来的逐像素合成；放大及 RGBA 路径使用下方行缓存。
            if(channels == 1 &&
               sourceHeight >= static_cast<int>(planY.offsets.size())) {
                const auto *xofs = planX.offsets.data();
                const auto *ialpha = planX.coefficients.data();
                const int rowLanes = sourceWidth * channels;
                for(int y = first; y < last; ++y) {
                    const int row0 = clipIndex(planY.offsets[y], sourceHeight);
                    const int row1 =
                        clipIndex(planY.offsets[y] + 1, sourceHeight);
                    const short b0 = planY.coefficients[y * 2];
                    const short b1 = planY.coefficients[y * 2 + 1];
                    const auto *s0 =
                        source + static_cast<std::size_t>(row0) * sourcePitch;
                    const auto *s1 =
                        source + static_cast<std::size_t>(row1) * sourcePitch;
                    auto *out = destination +
                        static_cast<std::size_t>(y) * destinationPitch;
                    for(int idx = 0; idx < width; ++idx) {
                        const int sx = xofs[idx];
                        // 边界区 a1 恒为 0，钳制仅用于避免越界读取。
                        const int next =
                            sx + channels < rowLanes ? sx + channels : sx;
                        const short a0 = ialpha[idx * 2],
                                    a1 = ialpha[idx * 2 + 1];
                        const std::int32_t left = s0[sx] * a0 + s0[next] * a1;
                        const std::int32_t right = s1[sx] * a0 + s1[next] * a1;
                        // 乘积有界：>>(4+16) 前因子不超过 32640*3072，int32
                        // 不溢出。
                        const std::int32_t sum = (((left >> 4) * b0) >> 16) +
                            (((right >> 4) * b1) >> 16);
                        out[idx] = satUchar((sum + 2) >> 2);
                    }
                }
                return;
            }
            // 每个同步任务独享两条行缓存，不保留像素且不在任务之间共享可变状态。
            std::vector<short> top(width), bottom(width);
            int topRow = -1, bottomRow = -1;
            for(int y = first; y < last; ++y) {
                const int row0 = clipIndex(planY.offsets[y], sourceHeight);
                const int row1 = clipIndex(planY.offsets[y] + 1, sourceHeight);
                if(row0 != topRow && row0 == bottomRow) {
                    top.swap(bottom);
                    std::swap(topRow, bottomRow);
                }
                if(row0 != topRow) {
                    horizontalLinear(
                        source + static_cast<std::size_t>(row0) * sourcePitch,
                        top.data(), width, sourceWidth, channels, planX);
                    topRow = row0;
                }
                if(row1 != row0 && row1 != bottomRow) {
                    horizontalLinear(
                        source + static_cast<std::size_t>(row1) * sourcePitch,
                        bottom.data(), width, sourceWidth, channels, planX);
                    bottomRow = row1;
                }
                verticalLinear(
                    top.data(), row1 == row0 ? top.data() : bottom.data(),
                    destination +
                        static_cast<std::size_t>(y) * destinationPitch,
                    width, planY.coefficients[y * 2],
                    planY.coefficients[y * 2 + 1]);
            }
        }

        int halfLinearVector(const std::uint8_t *top,
                             const std::uint8_t *bottom, std::uint8_t *out,
                             int width, int channels) {
            int x = 0;
#ifdef TVP_IMAGEOPS_SSE2
            const auto zero = _mm_setzero_si128();
            const auto rounding = _mm_set1_epi16(2);
            for(; x <= width - (channels == 4 ? 2 : 8);
                x += channels == 4 ? 2 : 8) {
                const int offset = x * 2 * channels;
                const auto a = _mm_loadu_si128(
                    reinterpret_cast<const __m128i *>(top + offset));
                const auto b = _mm_loadu_si128(
                    reinterpret_cast<const __m128i *>(bottom + offset));
                auto low = _mm_add_epi16(_mm_unpacklo_epi8(a, zero),
                                         _mm_unpacklo_epi8(b, zero));
                auto high = _mm_add_epi16(_mm_unpackhi_epi8(a, zero),
                                          _mm_unpackhi_epi8(b, zero));
                __m128i result;
                if(channels == 4) {
                    low = _mm_add_epi16(low, _mm_srli_si128(low, 8));
                    high = _mm_add_epi16(high, _mm_srli_si128(high, 8));
                    result = _mm_srli_epi16(
                        _mm_add_epi16(_mm_unpacklo_epi64(low, high), rounding),
                        2);
                } else {
                    const auto ones = _mm_set1_epi16(1);
                    const auto round32 = _mm_set1_epi32(2);
                    low = _mm_srli_epi32(
                        _mm_add_epi32(_mm_madd_epi16(low, ones), round32), 2);
                    high = _mm_srli_epi32(
                        _mm_add_epi32(_mm_madd_epi16(high, ones), round32), 2);
                    result = _mm_packs_epi32(low, high);
                }
                _mm_storel_epi64(
                    reinterpret_cast<__m128i *>(out + x * channels),
                    _mm_packus_epi16(result, result));
            }
#endif
            return x;
        }

        void resizeLinear(ConstImageView source, ImageView destination) {
            const auto sourcePitch = detail::Step(source);
            const auto destinationPitch = detail::Step(destination);
            const int width =
                destination.width * detail::Channels(source.format);
            if(source.width == destination.width &&
               source.height == destination.height) {
                if(source.data == destination.data)
                    return;
                const auto bytes =
                    detail::RowBytes(source.width, source.format);
                for(int y = 0; y < source.height; ++y)
                    std::memcpy(destination.data + y * destinationPitch,
                                source.data + y * sourcePitch, bytes);
                return;
            }
            // OpenCV 把恰好 2 倍的整数缩小交给了 INTER_AREA 的整数累加路径
            // （该路径与线性插值位等价），此处直接按其 2x2 均值复刻。
            if(source.width == 2LL * destination.width &&
               source.height == 2LL * destination.height) {
                const int channels = detail::Channels(source.format);
                detail::ForRows(
                    destination.height,
                    static_cast<std::size_t>(width) * destination.height,
                    [=](int first, int last) {
                        for(int y = first; y < last; ++y) {
                            const auto *top = source.data +
                                static_cast<std::size_t>(2 * y) * sourcePitch;
                            const auto *bottom = top + sourcePitch;
                            auto *out = destination.data +
                                static_cast<std::size_t>(y) * destinationPitch;
                            for(int dx = halfLinearVector(top, bottom, out,
                                                          destination.width,
                                                          channels);
                                dx < destination.width; ++dx) {
                                for(int ch = 0; ch < channels; ++ch) {
                                    const int o = dx * 2 * channels + ch;
                                    // OpenCV 的 fast_mode 2x2 用整数 (和+2)>>2
                                    // 半入，不是浮点四舍六入五取偶。
                                    const std::int32_t sum = top[o] +
                                        top[o + channels] + bottom[o] +
                                        bottom[o + channels];
                                    out[dx * channels + ch] =
                                        satUchar((sum + 2) >> 2);
                                }
                            }
                        }
                    });
                return;
            }

            LinearPlan planX(source.width, destination.width,
                             detail::Channels(source.format), false);
            LinearPlan planY(source.height, destination.height, 1, true);
            detail::ForRows(
                destination.height,
                static_cast<std::size_t>(width) * destination.height,
                [=, &planX, &planY](int first, int last) {
                    linearRows(source.data, destination.data, sourcePitch,
                               destinationPitch, width, source.width,
                               source.height, detail::Channels(source.format),
                               planX, planY, first, last);
                });
        }
    } // namespace


    ImageBuffer::ImageBuffer(int width, int height, PixelFormat format) :
        width_(width), height_(height), format_(format) {
        const auto row = detail::RowBytes(width, format);
        if(height <= 0 ||
           static_cast<std::size_t>(height) >
               static_cast<std::size_t>(
                   std::numeric_limits<std::ptrdiff_t>::max()) /
                   row)
            throw std::invalid_argument("ImageOps: invalid image height");
        pixels_.reset(new std::uint8_t[row * static_cast<std::size_t>(height)]);
    }

    std::ptrdiff_t ImageBuffer::pitch() const {
        return static_cast<std::ptrdiff_t>(detail::RowBytes(width_, format_));
    }
    ImageView ImageBuffer::view() {
        return { data(), width_, height_, pitch(), format_ };
    }
    ConstImageView ImageBuffer::view() const {
        return { data(), width_, height_, pitch(), format_ };
    }

    void Resize(ConstImageView source, ImageView destination,
                Interpolation mode) {
        if(source.format != destination.format)
            throw std::invalid_argument("ImageOps: mismatched pixel formats");
        switch(mode) {
            case Interpolation::Nearest:
                resizeNearest(source, destination);
                return;
            case Interpolation::Linear:
                resizeLinear(source, destination);
                return;
            case Interpolation::Area:
            case Interpolation::Cubic:
                detail::ResizeFiltered(source, destination, mode);
                return;
        }
        throw std::invalid_argument("ImageOps: unsupported interpolation");
    }
} // namespace TVPImageOps
