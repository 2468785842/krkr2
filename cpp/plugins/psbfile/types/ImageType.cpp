//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "ImageType.h"

#include "log/TVPLog.h"

namespace PSB {
    bool ImageType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        auto fdId = objects->find("id");
        if(fdId == objects->end())
            return false;
        std::string id =
            std::dynamic_pointer_cast<PSBString>(fdId->second)->value;

        return id == "image";
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    ImageType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        // TODO:
        G_PluginLog.critical("TODO: ImageType::collectResources(...)");
        return resourceList;
    }
} // namespace PSB