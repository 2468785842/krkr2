#include "HostFrame.h"

#include "tjsCommHead.h"
#include "Application.h"
#include "FrameStep.h"
#include "RenderManager.h"
#include "TVPTimer.h"

void TVPUpdateEngineFrame() {
    krkr::event::UpdateFrame([] { Application->Run(); },
                             iTVPTexture2D::RecycleProcess);
}

void TVPProcessEngineMessages() {
    krkr::event::MainThreadMessages().Dispatch();
    TVPTimer::ProgressAllTimer();
}
