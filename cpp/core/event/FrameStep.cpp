#include "FrameStep.h"
#include "PostUpdateEvent.h"

namespace krkr::event {
    void UpdateFrame(void (*runEngine)(), void (*recycleTextures)()) {
        runEngine();
        recycleTextures();
        TVPInvokePostUpdateEvent();
    }

    int ModalFramePacer::DrawIfDue(std::uint32_t now, int interval,
                                   void (*draw)(), void (*swapBuffers)()) {
        if(!lastDraw)
            lastDraw = now;

        // The platform clock wraps at 32 bits. Compare the unsigned elapsed
        // time before converting a bounded remaining delay back to int.
        const std::uint32_t elapsed = now - *lastDraw;
        if(interval > 0 && elapsed < static_cast<std::uint32_t>(interval))
            return interval - static_cast<int>(elapsed);

        TVPInvokePostUpdateEvent();
        draw();
        swapBuffers();
        lastDraw = now;
        return 0;
    }
} // namespace krkr::event
