#include "GLHost.h"
#include "CocosContextSubscription.h"
#include "base/CCDirector.h"
#include "base/CCConsole.h"
#include "renderer/ccGLStateCache.h"
#include <utility>

namespace krkr::glhost {
    void ActivateTexture(unsigned textureEnum) {
        cocos2d::GL::activeTexture(textureEnum);
    }
    void BindTexture2D(std::uint32_t name) { cocos2d::GL::bindTexture2D(name); }
    void BindTexture2DAt(unsigned unit, std::uint32_t name) {
        cocos2d::GL::bindTexture2DN(unit, name);
    }
    void DeleteTexture(std::uint32_t name) { cocos2d::GL::deleteTexture(name); }
    void UseProgram(std::uint32_t name) { cocos2d::GL::useProgram(name); }
    void EnableVertexAttributes(unsigned flags) {
        cocos2d::GL::enableVertexAttribs(flags);
    }
    void RestoreBlend() { cocos2d::GL::blendResetToCache(); }
    void RestoreViewport() { cocos2d::Director::getInstance()->setViewport(); }
    void Log(const char *message) { cocos2d::log("%s", message); }

    void CheckError(const char *file, const char *function, int line) {
        // Preserve the host's existing diagnostic mode and consumption of the
        // GL error flag. Release builds previously did not query glGetError.
#if defined(COCOS2D_DEBUG) && COCOS2D_DEBUG != 0
        if(auto error = glGetError())
            cocos2d::log("OpenGL error 0x%04X in %s %s %d\n", error, file,
                         function, line);
#else
        (void)file;
        (void)function;
        (void)line;
#endif
    }

    std::unique_ptr<ContextSubscription>
    ObserveContextRecreated(std::function<void()> callback) {
        return std::make_unique<cocosrender::ContextSubscription>(
            *cocos2d::Director::getInstance()->getEventDispatcher(),
            std::move(callback));
    }
} // namespace krkr::glhost
