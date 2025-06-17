#pragma once

#include <vector>
#include <memory>
#include "../resources/IResourceMetadata.h"
#include "../PSBEnums.h"

namespace PSB {

    class PSBFile;
    class IPSBType {
    public:
        virtual ~IPSBType() = default;
        virtual PSBType getPSBType() = 0;

        virtual bool isThisType(const PSBFile &psb) = 0;

        virtual std::vector<std::unique_ptr<IResourceMetadata>>
        collectResources(const PSBFile &psb, bool deDuplication) = 0;
    };
}; // namespace PSB