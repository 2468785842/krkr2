#pragma once

#include <map>

#include "types/IPSBType.h"
#include "types/MotionType.h"
#include "types/PimgType.h"
#include "types/ScnType.h"
#include "types/ImageType.h"
#include "types/MmoType.h"
#include "types/ArchiveType.h"
#include "types/SoundArchiveType.h"

namespace PSB {

    static const auto TypeHandlers = [] {
        std::map<PSBType, std::unique_ptr<IPSBType>> res{};
        res.emplace(PSBType::Motion, std::make_unique<MotionType>());
        res.emplace(PSBType::Scn, std::make_unique<ScnType>());
        res.emplace(PSBType::Tachie, std::make_unique<ImageType>());
        res.emplace(PSBType::Pimg, std::make_unique<PimgType>());
        res.emplace(PSBType::Mmo, std::make_unique<MmoType>());
        res.emplace(PSBType::ArchiveInfo, std::make_unique<ArchiveType>());
        res.emplace(PSBType::SoundArchive,
                    std::make_unique<SoundArchiveType>());
        // {PSBType::BmpFont, FontType{}},
        // {PSBType::Map, MapType{}},
        // {PSBType::ClutImg, ClutType{}},
        // {PSBType::SprBlock, SprBlockType{}},
        // {PSBType::SprData, SprDataType{}},
        // {PSBType::Chip, ChipType{}},
        return res;
    }();
} // namespace PSB