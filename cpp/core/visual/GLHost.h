#pragma once

#include <cstdint>
#include <functional>
#include <memory>

// All calls execute in the existing render thread/current shared GL context.
// The host implementation must keep its GL state cache coherent with the
// renderer; replacing these calls with raw GL calls is not equivalent.
namespace krkr::glhost {
    void ActivateTexture(unsigned textureEnum);
    void BindTexture2D(std::uint32_t name);
    void BindTexture2DAt(unsigned unit, std::uint32_t name);
    void DeleteTexture(std::uint32_t name);
    void UseProgram(std::uint32_t name);
    void EnableVertexAttributes(unsigned flags);
    void RestoreBlend();
    void RestoreViewport();
    void Log(const char *message);

    class ContextSubscription {
    public:
        virtual ~ContextSubscription() = default;
    };

    // Owns the registration, not the GL context. Destroy on the event/render
    // thread before the callback owner. Notification retains the old backend's
    // shader-rebuild semantics; it does not promise texture/FBO restoration.
    std::unique_ptr<ContextSubscription>
    ObserveContextRecreated(std::function<void()> callback);
} // namespace krkr::glhost
