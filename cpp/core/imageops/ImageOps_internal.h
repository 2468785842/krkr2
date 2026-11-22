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
        // 标准 C++17 调度；小图串行，大图按线程上限分片，异常在调用方重抛。
        void ForRows(int height, std::size_t pixels,
                     const std::function<void(int, int)> &task);
        // Area/Cubic 的独立实现；Nearest 与 Linear 位于 ImageOps.cpp。
        void ResizeFiltered(ConstImageView source, ImageView destination,
                            Interpolation mode);
    } // namespace detail
} // namespace TVPImageOps
