#pragma once

#include <cstdint>
#include <optional>

namespace krkr::event {
    // Run game logic and recycle textures before notifying the frame hook.
    void UpdateFrame(void (*runEngine)(), void (*recycleTextures)());

    class ModalFramePacer {
    public:
        // Times and intervals are in milliseconds. Nonpositive intervals
        // force one frame. A positive return value is the delay until due.
        int DrawIfDue(std::uint32_t now, int interval, void (*draw)(),
                      void (*swapBuffers)());

    private:
        std::optional<std::uint32_t> lastDraw;
    };
} // namespace krkr::event
