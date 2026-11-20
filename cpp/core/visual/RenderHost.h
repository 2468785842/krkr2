#pragma once

#include "RenderSettings.h"
#include <string>

namespace krkr::renderhost {
    std::string RendererName();
    render::TextureStorage SoftwareStorage();
    render::TextureStorage GLStorage();
    int TextureLimit();
    bool ExtensionAllowed(const char *name);
    std::string SupportedExtensionsLabel();
    void CheckMemory();
    void ConsoleLog(const char *message);
    void ReportTextureTooLarge();
    void ReportUnsupportedOperation(const char *operation,
                                    const char *renderer);
} // namespace krkr::renderhost
