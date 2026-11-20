#include "RenderHost.h"

#include "tjsCommHead.h"
#include "Platform.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "ConfigManager/LocaleConfigManager.h"

namespace TJS {
    void TVPConsoleLog(const ttstr &message);
}

namespace {
    class PreferenceSource final : public krkr::render::PreferenceSource {
    public:
        std::string String(const char *key, const char *fallback) override {
            return IndividualConfigManager::GetInstance()
                ->GetValue<std::string>(key, fallback);
        }
        int Integer(const char *key, int fallback) override {
            return IndividualConfigManager::GetInstance()->GetValue<int>(
                key, fallback);
        }
    };
    PreferenceSource &Preferences() {
        static PreferenceSource source;
        return source;
    }
} // namespace

namespace krkr::renderhost {
    std::string RendererName() { return render::RendererName(Preferences()); }
    render::TextureStorage SoftwareStorage() {
        return render::SoftwareStorage(Preferences());
    }
    render::TextureStorage GLStorage() {
        return render::GLStorage(Preferences());
    }
    int TextureLimit() { return Preferences().Integer("ogl_max_texsize", 0); }
    bool ExtensionAllowed(const char *name) {
        return Preferences().Integer(name, 1) != 0;
    }
    std::string SupportedExtensionsLabel() {
        return LocaleConfigManager::GetInstance()->GetText(
            "supported_opengl_extension");
    }
    void CheckMemory() { TVPCheckMemory(); }
    void ConsoleLog(const char *message) { TJS::TVPConsoleLog(ttstr(message)); }
    void ReportTextureTooLarge() {
        TVPShowSimpleMessageBox(
            ttstr(TJS_W("Too large texture is not accessable by GPU.")),
            TJS_W("Please use software renderer"));
    }
    void ReportUnsupportedOperation(const char *operation,
                                    const char *renderer) {
        TVPShowSimpleMessageBox(ttstr(TJS_W("Operation \"")) + operation +
                                    TJS_W("\" is not supported under ") +
                                    renderer + TJS_W(" mode."),
                                TJS_W("Please use software renderer"));
    }
} // namespace krkr::renderhost
