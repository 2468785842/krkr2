#pragma once

#include <string>

#include "IPSBType.h"
#include "BaseImageType.h"
#include "../resources/ImageMetadata.h"

namespace PSB {

    class ImageType : public BaseImageType, public IPSBType {
    public:
        static inline const std::string &G_ImageSourceKey = "imageList";

        PSBType getPSBType() override { return PSBType::Tachie; }

        bool isThisType(const PSBFile &psb) override;

        std::vector<std::unique_ptr<IResourceMetadata>>
        collectResources(const PSBFile &psb, bool deDuplication) override;
    };
}; // namespace PSB