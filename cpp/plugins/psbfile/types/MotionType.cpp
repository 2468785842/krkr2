//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "MotionType.h"

namespace PSB {

#define LOGGER spdlog::get("plugin")

    bool MotionType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        auto fdId = objects->find("id");
        return fdId != objects->end() &&
            std::dynamic_pointer_cast<PSBString>(fdId->second)->value ==
            "motion";
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    MotionType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        // TODO:
        LOGGER->critical("TODO: MotionType::collectResources(...)");
        return resourceList;
    }
} // namespace PSB