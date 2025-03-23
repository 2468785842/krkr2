#pragma once

namespace PSB {
    enum class PSBType {
        // Unknown type PSB
        PSB = 0,
        // Images (pimg, dpak)
        Pimg = 1,
        // Script (scn)
        // TODO: KS decompiler?
        Scn = 2,
        // EMT project - M2 MOtion (mmo, emtproj)
        Mmo = 3,
        // Images with Layouts (used in PS*)
        Tachie = 4,
        // MDF Archive Index (_info.psb.m)
        ArchiveInfo = 5,
        // BMP Font (e.g. textfont24)
        BmpFont = 6,
        // EMT
        Motion = 7,
        // Sound Archive
        SoundArchive = 8,
        // Tile Map
        Map = 9,
        // Sprite Block
        SprBlock = 10,
        // Sprite Data (define)
        SprData = 11,
        // CLUT - Images with Color Look-Up Table
        ClutImg = 12,
        // Chip
        Chip = 13
    };
};