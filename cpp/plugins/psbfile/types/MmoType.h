#pragma once

#include "IPSBType.h"
#include "BaseImageType.h"
#include "../resources/ImageMetadata.h"

namespace PSB {

    class MmoType : public BaseImageType, public IPSBType {
    public:
        static inline const std::string &G_MmoSourceKey = "sourceChildren";
        static inline const std::string &G_MmoBgSourceKey = "bgChildren";

        PSBType getPSBType() override { return PSBType::Mmo; }

        bool isThisType(const PSBFile &psb) override;

        std::vector<std::unique_ptr<IResourceMetadata>>
        collectResources(const PSBFile &psb, bool deDuplication) override;
    };
}; // namespace PSB