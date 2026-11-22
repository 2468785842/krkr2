#pragma once

#include <cstdint>
#include <functional>
#include <memory>

class iWindowLayer;

namespace krkr::video {
    // Tightly packed YUV420P planes, borrowed for the duration of Present.
    struct Yuv420Frame {
        const std::uint8_t *y;
        const std::uint8_t *u;
        const std::uint8_t *v;
        int width;
        int height;
    };

    // Bounds in parent coordinates, measured from the top left.
    struct Bounds {
        int left;
        int top;
        int width;
        int height;
    };

    // Owned by the player and used on the host/render thread. Destroying the
    // surface detaches it and cancels its frame callback.
    class Presentation {
    public:
        virtual ~Presentation() = default;
        virtual void SetVisible(bool visible) = 0;
        virtual void SetPictureVisible(bool visible) = 0;
        virtual void Present(const Yuv420Frame &frame,
                             const Bounds &bounds) = 0;
    };

    using FrameCallback = std::function<void(float)>;

    std::unique_ptr<Presentation>
    CreateWindowPresentation(iWindowLayer &window, FrameCallback onFrame);
} // namespace krkr::video
