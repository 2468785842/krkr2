#pragma once

#include <unordered_map>

#include "types/IPSBType.h"
#include "types/PimgType.h"

namespace PSB {

    static const std::unordered_map<PSBType, IPSBType *> TypeHandlers{
        // {PSBType::Motion, MotionType{}},
        // {PSBType::Scn, ScnType{}},
        // {PSBType::Tachie, ImageType{}},
        { PSBType::Pimg, new PimgType{} },
        // {PSBType::Mmo, MmoType{}},
        // {PSBType::ArchiveInfo, ArchiveType{}},
        // {PSBType::SoundArchive, SoundArchiveType{}},
        // {PSBType::BmpFont, FontType{}},
        // {PSBType::Map, MapType{}},
        // {PSBType::ClutImg, ClutType{}},
        // {PSBType::SprBlock, SprBlockType{}},
        // {PSBType::SprData, SprDataType{}},
        // {PSBType::Chip, ChipType{}},
        // {PSBType::PSB, MotionType{}}, //assume as motion type by default,
        // must put this after Motion
    };
}