//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "MmoType.h"

namespace PSB {
#define LOGGER spdlog::get("plugin")

    bool MmoType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }
        auto fdOC = objects->find("objectChildren");
        auto fdSC = objects->find("sourceChildren");
        return fdOC != objects->end() && fdSC != objects->end();
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    MmoType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        // TODO:
        LOGGER->critical("TODO: MmoType::collectResources(...)");
        return resourceList;
    }
} // namespace PSB