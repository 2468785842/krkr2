#include "ImageOps_internal.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

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
            case Interpolation::Area:
            case Interpolation::Linear:
            case Interpolation::Cubic:
                detail::ResizeFiltered(source, destination, mode);
                return;
        }
        throw std::invalid_argument("ImageOps: unsupported interpolation");
    }
} // namespace TVPImageOps
