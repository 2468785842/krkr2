#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace TVPImageOps {

    enum class PixelFormat { Gray8 = 1, RGBA8 = 4 };
    enum class Interpolation { Nearest = 0, Area = 1, Linear = 2, Cubic = 3 };

    // 视图借用调用方缓冲区，不保留指针；pitch 按字节计，0 表示紧密排列。
    // 支持正向行布局；缓冲区须覆盖 (height-1)*pitch + width*通道数。
    // 四通道按原始字节独立处理，不交换颜色、不预乘或反预乘 alpha。
    struct ConstImageView {
        const std::uint8_t *data;
        int width, height;
        std::ptrdiff_t pitch;
        PixelFormat format;
    };

    struct ImageView {
        std::uint8_t *data;
        int width, height;
        std::ptrdiff_t pitch;
        PixelFormat format;
        operator ConstImageView() const {
            return { data, width, height, pitch, format };
        }
    };

    struct Point {
        float x, y;
        Point(float x, float y) : x(x), y(y) {}
    };
    using Quad = std::array<Point, 4>;
    using Matrix3 = std::array<double, 9>;

    class ImageBuffer {
        int width_, height_;
        PixelFormat format_;
        std::unique_ptr<std::uint8_t[]> pixels_;

    public:
        ImageBuffer(int width, int height, PixelFormat format);
        int width() const { return width_; }
        int height() const { return height_; }
        std::ptrdiff_t pitch() const;
        // 可移动、不可复制。初始像素未初始化；Warp 操作会写满输出（含边界）。
        std::uint8_t *data() { return pixels_.get(); }
        const std::uint8_t *data() const { return pixels_.get(); }
        ImageView view();
        ConstImageView view() const;
    };

    // 参数错误抛 std::invalid_argument，后端错误转换为 std::runtime_error。
    // Resize 保留既有 Nearest/Area/Linear/Cubic 映射及像素坐标、舍入语义。
    // Nearest 为原生实现；输入输出缓冲区应独立，允许相同视图的原位恒等操作。
    void Resize(ConstImageView source, ImageView destination,
                Interpolation mode);

    // 与既有 boxFilter 一致：归一化、中心 anchor（偶数核向下取整）、REFLECT_101
    // 边界。 边界相对当前视图，输出尺寸及格式必须等于输入；支持原位操作。
    void BoxBlur(ConstImageView source, ImageView destination, int kernelWidth,
                 int kernelHeight);

    // 输入点采用 float，矩阵为行优先 double；保持旧 Point2f 的精度与顶点顺序。
    Matrix3 PerspectiveTransform(const Quad &source, const Quad &destination);

    // 保持既有零常量边界和正向变换语义；输出独立持有紧密排列的像素。
    // Affine 仅用前三个点，与旧四点数组传入 getAffineTransform 的行为一致。
    ImageBuffer WarpAffine(ConstImageView source, int width, int height,
                           const Quad &sourcePoints,
                           const Quad &destinationPoints, Interpolation mode);
    ImageBuffer WarpPerspective(ConstImageView source, int width, int height,
                                const Quad &sourcePoints,
                                const Quad &destinationPoints,
                                Interpolation mode);

} // namespace TVPImageOps
