#include "CocosTexturePresentation.h"

#include "tjsCommHead.h"
#include "RenderManager.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCTexture2D.h"
#include <stdexcept>

namespace {
    class SharedGLTexture final : public cocos2d::Texture2D {
        krkr::render::TextureLease sourceLease;

    public:
        SharedGLTexture(iTVPTexture2D &source,
                        const krkr::render::OpenGLFrame &frame) {
            sourceLease.Reset(source);
            _contentSize = cocos2d::Size(frame.width, frame.height);
            _maxS = _maxT = 1;
            _pixelsWide = frame.width;
            _pixelsHigh = frame.height;
            _pixelFormat = PixelFormat::RGBA8888;
            _hasPremultipliedAlpha = false;
            _hasMipmaps = false;
            setGLProgram(cocos2d::GLProgramCache::getInstance()->getGLProgram(
                cocos2d::GLProgram::SHADER_NAME_POSITION_TEXTURE));
            _name = frame.name;
        }

        ~SharedGLTexture() override {
            // Texture2D's destructor must not delete the source's GL name.
            _name = 0;
        }

        void Update(iTVPTexture2D &source,
                    const krkr::render::OpenGLFrame &frame) {
            sourceLease.Reset(source);
            _name = frame.name;
        }
    };
} // namespace

cocos2d::Texture2D *
TVPGetCocosPresentationTexture(iTVPTexture2D &source,
                               cocos2d::Texture2D *previous) {
    const auto frame = source.GetPresentationFrame();
    if(const auto *gl = std::get_if<krkr::render::OpenGLFrame>(&frame)) {
        auto *shared = dynamic_cast<SharedGLTexture *>(previous);
        if(shared && shared->getPixelsWide() == gl->width &&
           shared->getPixelsHigh() == gl->height) {
            shared->Update(source, *gl);
            return shared;
        }
        auto *texture = new SharedGLTexture(source, *gl);
        texture->autorelease();
        return texture;
    }

    const auto &cpu = std::get<krkr::render::RGBAFrame>(frame);
    const bool contiguous =
        !cpu.readRow && cpu.pitch == static_cast<std::ptrdiff_t>(cpu.width) * 4;
    auto *texture = previous;
    // A borrowed GL adapter must never become the destination of a CPU upload.
    if(!texture || dynamic_cast<SharedGLTexture *>(texture) ||
       texture->getPixelsWide() != cpu.width ||
       texture->getPixelsHigh() != cpu.height) {
        texture = new cocos2d::Texture2D;
        texture->autorelease();
        if(!texture->initWithData(contiguous ? cpu.pixels : nullptr,
                                  static_cast<std::size_t>(cpu.width) *
                                      cpu.height * 4,
                                  cocos2d::Texture2D::PixelFormat::RGBA8888,
                                  cpu.width, cpu.height, cocos2d::Size::ZERO))
            throw std::runtime_error("Cannot allocate presentation texture");
        if(contiguous)
            return texture;
    }
    if(contiguous) {
        texture->updateWithData(cpu.pixels, 0, 0, cpu.width, cpu.height);
    } else {
        for(unsigned y = 0; y < cpu.height; ++y)
            texture->updateWithData(cpu.Row(y), 0, y, cpu.width, 1);
    }
    return texture;
}
