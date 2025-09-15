//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "SoundArchiveType.h"

namespace PSB {

#define LOGGER spdlog::get("plugin")

    bool SoundArchiveType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        auto fdId = objects->find("id");
        auto str = std::dynamic_pointer_cast<PSBString>(fdId->second);
        return fdId != objects->end() && str != nullptr &&
            str->value == "sound_archive";
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    SoundArchiveType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        // TODO:
        LOGGER->critical("TODO: SoundArchiveType::collectResources(...)");
        return resourceList;
    }
} // namespace PSB