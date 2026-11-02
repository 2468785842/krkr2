#include "ImageOps.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace TVPImageOps {
    namespace {
        int channels(PixelFormat format) {
            switch(format) {
                case PixelFormat::Gray8:
                    return 1;
                case PixelFormat::RGBA8:
                    return 4;
            }
            throw std::invalid_argument("ImageOps: unsupported pixel format");
        }

        std::size_t rowBytes(int width, PixelFormat format) {
            if(width <= 0 ||
               static_cast<std::size_t>(width) >
                   static_cast<std::size_t>(
                       std::numeric_limits<std::ptrdiff_t>::max()) /
                       channels(format))
                throw std::invalid_argument("ImageOps: invalid image width");
            return static_cast<std::size_t>(width) * channels(format);
        }

        std::size_t step(ConstImageView image) {
            const auto row = rowBytes(image.width, image.format);
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

        cv::Mat matrix(ConstImageView image) {
            const auto stride = step(image);
            return { image.height, image.width,
                     CV_MAKETYPE(CV_8U, channels(image.format)),
                     const_cast<std::uint8_t *>(image.data), stride };
        }

        int interpolation(Interpolation mode) {
            switch(mode) {
                case Interpolation::Nearest:
                    return cv::INTER_NEAREST;
                case Interpolation::Area:
                    return cv::INTER_AREA;
                case Interpolation::Linear:
                    return cv::INTER_LINEAR;
                case Interpolation::Cubic:
                    return cv::INTER_CUBIC;
            }
            throw std::invalid_argument("ImageOps: unsupported interpolation");
        }

        std::array<cv::Point2f, 4> points(const Quad &quad) {
            return { cv::Point2f(quad[0].x, quad[0].y),
                     cv::Point2f(quad[1].x, quad[1].y),
                     cv::Point2f(quad[2].x, quad[2].y),
                     cv::Point2f(quad[3].x, quad[3].y) };
        }

        template <typename Function>
        auto backend(Function &&function) -> decltype(function()) {
            try {
                return function();
            } catch(const cv::Exception &error) {
                throw std::runtime_error(error.what());
            }
        }
    } // namespace

    ImageBuffer::ImageBuffer(int width, int height, PixelFormat format) :
        width_(width), height_(height), format_(format) {
        const auto row = rowBytes(width, format);
        if(height <= 0 ||
           static_cast<std::size_t>(height) >
               static_cast<std::size_t>(
                   std::numeric_limits<std::ptrdiff_t>::max()) /
                   row)
            throw std::invalid_argument("ImageOps: invalid image height");
        pixels_.reset(new std::uint8_t[row * static_cast<std::size_t>(height)]);
    }

    std::ptrdiff_t ImageBuffer::pitch() const {
        return static_cast<std::ptrdiff_t>(rowBytes(width_, format_));
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
        backend([&] {
            auto src = matrix(source), dst = matrix(destination);
            cv::resize(src, dst, dst.size(), 0, 0, interpolation(mode));
        });
    }

    void BoxBlur(ConstImageView source, ImageView destination, int kernelWidth,
                 int kernelHeight) {
        if(source.width != destination.width ||
           source.height != destination.height ||
           source.format != destination.format || kernelWidth <= 0 ||
           kernelHeight <= 0)
            throw std::invalid_argument(
                "ImageOps: invalid box blur dimensions");
        backend([&] {
            auto src = matrix(source), dst = matrix(destination);
            cv::boxFilter(src, dst, -1, { kernelWidth, kernelHeight });
        });
    }

    Matrix3 PerspectiveTransform(const Quad &source, const Quad &destination) {
        return backend([&] {
            auto from = points(source), to = points(destination);
            auto transform =
                cv::getPerspectiveTransform(from.data(), to.data());
            Matrix3 result;
            std::copy_n(transform.ptr<double>(), result.size(), result.begin());
            return result;
        });
    }

    ImageBuffer WarpAffine(ConstImageView source, int width, int height,
                           const Quad &sourcePoints,
                           const Quad &destinationPoints, Interpolation mode) {
        return backend([&] {
            auto src = matrix(source);
            ImageBuffer result(width, height, source.format);
            auto dst = matrix(result.view());
            auto from = points(sourcePoints), to = points(destinationPoints);
            auto transform = cv::getAffineTransform(from.data(), to.data());
            cv::warpAffine(src, dst, transform, dst.size(),
                           interpolation(mode));
            return result;
        });
    }

    ImageBuffer WarpPerspective(ConstImageView source, int width, int height,
                                const Quad &sourcePoints,
                                const Quad &destinationPoints,
                                Interpolation mode) {
        return backend([&] {
            auto src = matrix(source);
            ImageBuffer result(width, height, source.format);
            auto dst = matrix(result.view());
            auto from = points(sourcePoints), to = points(destinationPoints);
            auto transform =
                cv::getPerspectiveTransform(from.data(), to.data());
            cv::warpPerspective(src, dst, transform, dst.size(),
                                interpolation(mode));
            return result;
        });
    }
} // namespace TVPImageOps
