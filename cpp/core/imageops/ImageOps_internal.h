#pragma once

#include "ImageOps.h"

namespace TVPImageOps {
    // 模块内部共享校验；公共视图与所有权实现不依赖后端头文件。
    namespace detail {
        int Channels(PixelFormat format);
        std::size_t RowBytes(int width, PixelFormat format);
        std::size_t Step(ConstImageView image);
        void ResizeFiltered(ConstImageView source, ImageView destination,
                            Interpolation mode);
    } // namespace detail
} // namespace TVPImageOps
