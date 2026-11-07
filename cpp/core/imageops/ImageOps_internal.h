#pragma once

#include "ImageOps.h"
#include <functional>

namespace TVPImageOps {
    // 模块内部共享校验；公共视图与所有权实现不依赖后端头文件。
    namespace detail {
        int Channels(PixelFormat format);
        std::size_t RowBytes(int width, PixelFormat format);
        std::size_t Step(ConstImageView image);
        // 同步执行全部行任务；不保留回调或捕获的视图/列索引。
        // 暂沿用后端调度，保留线程数和大图分片行为，便于单独迁移像素算法。
        void ForRows(int height, std::size_t pixels,
                     const std::function<void(int, int)> &task);
        void ResizeFiltered(ConstImageView source, ImageView destination,
                            Interpolation mode);
    } // namespace detail
} // namespace TVPImageOps
