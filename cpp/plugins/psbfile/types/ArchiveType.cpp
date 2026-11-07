//
// Created by LiDon on 2025/9/15.
//

#include "../PSBFile.h"
#include "ArchiveType.h"

namespace PSB {

    bool ArchiveType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        auto fdId = objects->find("id");
        if(fdId == objects->end())
            return false;
        auto value = std::dynamic_pointer_cast<PSBString>(fdId->second);
        if(!value)
            return false;
        const auto &id = value->value;

        return id == "archive" ||
            (id == "scenario" && objects->find("file_info") != objects->end());
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    ArchiveType::collectResources(const PSBFile &psb, bool deDuplication) {
        return {};
    }
} // namespace PSB