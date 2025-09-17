//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "ScnType.h"

namespace PSB {

#define LOGGER spdlog::get("plugin")

    bool ScnType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        if(objects->find("scenes") != objects->end() &&
           objects->find("name") != objects->end()) {
            return true;
        }

        if(objects->find("list") != objects->end() &&
           objects->find("map") != objects->end() && !psb.resources.empty()) {
            return true;
        }

        return false;
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    ScnType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        // TODO:
        LOGGER->critical("TODO: ScnType::collectResources(...)");
        return resourceList;
    }
} // namespace PSB