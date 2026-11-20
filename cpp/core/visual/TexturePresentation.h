#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>

namespace krkr::render {

    // Borrowed until the synchronous upload returns. Pixels contain RGBA bytes;
    // pitch includes padding and may be negative. A row reader handles
    // compressed storage without requiring a contiguous temporary bitmap.
    struct RGBAFrame {
        unsigned width;
        unsigned height;
        const void *pixels;
        std::ptrdiff_t pitch;
        const void *rowContext = nullptr;
        const void *(*readRow)(const void *, unsigned) = nullptr;

        const void *Row(unsigned y) const {
            if(readRow)
                return readRow(rowContext, y);
            return static_cast<const std::uint8_t *>(pixels) +
                pitch * static_cast<std::ptrdiff_t>(y);
        }
    };

    // A borrowed name in the current shared GL context. The host must retain
    // the source texture while using it and must never delete this GL name
    // itself.
    struct OpenGLFrame {
        std::uint32_t name;
        unsigned width;
        unsigned height;
    };

    using TextureFrame = std::variant<RGBAFrame, OpenGLFrame>;

    // Host-side ownership of a source with AddRef/Release. Retain the
    // replacement before releasing the previous source, including when the
    // latter owns it.
    class TextureLease {
        void *owner = nullptr;
        void (*release)(void *) = nullptr;

    public:
        TextureLease() = default;
        TextureLease(const TextureLease &) = delete;
        TextureLease &operator=(const TextureLease &) = delete;
        ~TextureLease() { Reset(); }

        void Reset() {
            auto *previous = owner;
            auto previousRelease = release;
            owner = nullptr;
            release = nullptr;
            if(previous)
                previousRelease(previous);
        }

        template <typename T>
        void Reset(T &source) {
            if(owner == &source)
                return;
            source.AddRef();
            Reset();
            owner = &source;
            release = [](void *p) { static_cast<T *>(p)->Release(); };
        }
    };

} // namespace krkr::render
