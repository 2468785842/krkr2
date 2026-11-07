#include "krffmpeg.h"

#include "MsgIntf.h"
#include <mutex>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
}

static int lockmgr(void **arg, AVLockOp op) {
    auto **mtx = (std::mutex **)arg;
    switch(op) {
        case AV_LOCK_CREATE:
            *mtx = new std::mutex();
            if(!*mtx)
                return 1;
            break;
        case AV_LOCK_OBTAIN:
            (*mtx)->lock();
            break;
        case AV_LOCK_RELEASE:
            (*mtx)->unlock();
            break;
        case AV_LOCK_DESTROY:
            delete *mtx;
            break;
        default:
            return 1;
    }
    return 0;
}

static bool FFInitilalized = false;

void TVPInitLibAVCodec() {
    if(!FFInitilalized) {
        if(av_lockmgr_register(lockmgr)) {
            TVPThrowExceptionMessage(
                TJS_W("Could not initialize lock manager!"));
        }
        avcodec_register_all();
        av_register_all();
        avfilter_register_all();
        avformat_network_init();
        FFInitilalized = true;
    }
}
