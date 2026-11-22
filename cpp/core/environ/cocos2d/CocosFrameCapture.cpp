#include "FrameCapture.h"

#include "base/ccUtils.h"
#include <utility>

void TVPCaptureHostFrame(const std::string &path,
                         std::function<void(bool)> completion) {
    cocos2d::utils::captureScreen(
        [completion = std::move(completion)](bool saved, const std::string &) {
            completion(saved);
        },
        path);
}
