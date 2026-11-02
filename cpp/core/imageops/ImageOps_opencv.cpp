#include "ImageOps_internal.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <stdexcept>

namespace TVPImageOps {
    namespace {
        cv::Mat matrix(ConstImageView image) {
            const auto stride = detail::Step(image);
            return { image.height, image.width,
                     CV_MAKETYPE(CV_8U, detail::Channels(image.format)),
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

    void detail::ForRows(int height, std::size_t pixels,
                         const std::function<void(int, int)> &task) {
        backend([&] {
            cv::parallel_for_(
                cv::Range(0, height),
                [&](const cv::Range &rows) { task(rows.start, rows.end); },
                static_cast<double>(pixels) / (1 << 16));
        });
    }

    void detail::ResizeFiltered(ConstImageView source, ImageView destination,
                                Interpolation mode) {
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
