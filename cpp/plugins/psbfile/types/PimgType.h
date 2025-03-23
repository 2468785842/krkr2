#pragma once

#include <string>

#include "IPSBType.h"
#include "BaseImageType.h"

namespace PSB {

    class PimgType : public BaseImageType, public IPSBType {
    public:
        const std::string PimgSourceKey = "layers";

        PSBType getPSBType() override { return PSBType::Pimg; }

        bool isThisType(const PSBFile &psb) override;
    };
}; // namespace PSB