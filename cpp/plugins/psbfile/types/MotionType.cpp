//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "MotionType.h"

#include "log/TVPLog.h"

namespace PSB {
    bool MotionType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        auto fdId = objects->find("id");
        if(fdId == objects->end())
            return false;
        auto id = std::dynamic_pointer_cast<PSBString>(fdId->second);
        return id && id->value == "motion";
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    MotionType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        // TODO:
        G_PluginLog.critical("TODO: MotionType::collectResources(...)");
        return resourceList;
    }
} // namespace PSB
