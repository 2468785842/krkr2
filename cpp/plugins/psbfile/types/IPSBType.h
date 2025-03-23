#pragma once

#include "../PSBEnums.h"

namespace PSB {

    class PSBFile;

    class IPSBType {
    public:
        virtual PSBType getPSBType() = 0;

        virtual bool isThisType(const PSBFile &psb) = 0;

        // T must be class IResourceMetadata
        // template<typename T>
        // std::vector<T> collectResources(PSBFile psb, bool deDuplication =
        // true);
    };
}; // namespace PSB