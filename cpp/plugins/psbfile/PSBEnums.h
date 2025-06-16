#pragma once

#include <limits>
#include <string>
#include <vector>

#include <boost/locale.hpp>

namespace PSB {

    static void split(const std::string &s, std::vector<std::string> &tokens,
                      const std::string &delimiters = ",") {
        std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
        std::string::size_type pos = s.find_first_of(delimiters, lastPos);
        while(std::string::npos != pos || std::string::npos != lastPos) {
            tokens.emplace_back(s.substr(lastPos, pos - lastPos));
            lastPos = s.find_first_not_of(delimiters, pos);
            pos = s.find_first_of(delimiters, lastPos);
        }
    }

#define DECLARE_ENUM(EnumName, ...)                                            \
    enum class EnumName { __VA_ARGS__, _COUNT };                               \
    static const char G_##EnumName##Strings[] = { #__VA_ARGS__ };              \
    static std::vector<std::string> G_##EnumName##Vector{};                    \
    static EnumName tryParseTo##EnumName##Enum(                                \
        const std::string_view &enumString, bool ignoreCase) {                 \
        if(G_##EnumName##Vector.empty()) {                                     \
            split(G_##EnumName##Strings, G_##EnumName##Vector);                \
        }                                                                      \
        for(size_t i = 0; i < G_##EnumName##Vector.size(); i++) {              \
            std::string tmp = G_##EnumName##Vector[i];                         \
            if(ignoreCase) {                                                   \
                tmp = boost::locale::to_upper(tmp);                            \
            }                                                                  \
            if(tmp == enumString) {                                            \
                return static_cast<EnumName>(i);                               \
            }                                                                  \
        }                                                                      \
        return EnumName::None;                                                 \
    }

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

    /**
     * EMT PSB Platform
     */
    enum class PSBSpec : unsigned char {
        // Do not have spec
        None = std::numeric_limits<unsigned char>::min(),
        // Unity and other
        Common,
        // Kirikiri
        Krkr,
        // DirectX
        Win,
        // WebGL
        Ems,
        // PSP
        PSP,
        // PS Vita
        Vita,
        // PS3
        PS3,
        // PS4
        PS4,
        // N Switch
        NX,
        // 3DS
        Citra,
        // Android
        And,
        // XBox 360
        X360,
        // N Wii (Revolution)
        Revo,

        Other = std::numeric_limits<unsigned char>::max(),
    };

    enum class PSBImageFormat {
        png,
        bmp,
    };

    DECLARE_ENUM(
        PSBPixelFormat, None,
        /// Little Endian RGBA8 (plat: win)
        LeRGBA8,
        /// Big Endian RGBA8 (plat: common)
        BeRGBA8,
        /// Little Endian RGBA4444 (plat: win)
        LeRGBA4444,
        /// Big Endian RGBA4444 (plat: common)
        BeRGBA4444,
        /// RGBA5650
        RGBA5650,
        /// BE RGB5A3 (plat: revo, aka. 5553)
        RGB5A3,
        /// A8L8
        A8L8,
        /// L8 (L = Lightness)
        L8,
        /// A8 (A = Alpha)
        A8,

        // SW and Tile

        /// BeRGBA8_SW (Swizzle) for vita
        BeRGBA8_SW,
        /// LeRGBA8_SW (Swizzle) for vita
        LeRGBA8_SW,
        ///// LeRGBX8_SW (Swizzle, Flip) for PS3 //map to RGBA8_SW
        // LeRGBX8_SW,
        /// LeRGBA8_SW (Swizzle, Flip) for PS3?
        FlipLeRGBA8_SW,
        /// BeRGBA8_SW (Swizzle, Flip) for PS3
        FlipBeRGBA8_SW,
        /// LeRGBA8_SW (Swizzle, Tile) for PS4
        TileLeRGBA8_SW,
        /// BeRGBA8_SW (Swizzle, Tile) for ?
        TileBeRGBA8_SW,
        /// Tile RGBA8 for Wii
        TileBeRGBA8_Rvl,
        /// Little Endian RGBA4444 with Swizzle for vita
        LeRGBA4444_SW,
        /// RGBA4444_SW (Swizzle, Tile) for PS4
        TileLeRGBA4444_SW,
        /// RGBA5650 with Swizzle for vita
        RGBA5650_SW,
        /// RGBA5650 with Tile for PS4
        TileRGBA5650_SW,
        /// A8L8 with Swizzle for vita
        A8L8_SW,
        /// A8L8_SW (Tile) for PS4
        TileA8L8_SW,
        /// L8 with Swizzle for vita
        L8_SW,
        /// L8_SW (Tile) for PS4
        TileL8_SW,
        /// A8_SW (Swizzle)
        A8_SW,
        /// A8_SW (Tile) for PS4
        TileA8_SW,
        /// CI8 (without swizzle)
        CI8,
        /// CI8 (C8) with Swizzle for vita
        /// REF: http://wiki.tockdom.com/wiki/Image_Formats#C8_.28CI8.29
        CI8_SW,
        /// CI8 with Swizzle for PSP
        CI8_SW_PSP,
        /// CI8 (Tile)
        /// for Wii
        TileCI8,
        /// CI4 (without swizzle)
        CI4,
        /// CI4 with Swizzle for vita
        CI4_SW,
        /// CI4 with Swizzle for PSP
        CI4_SW_PSP,
        /// CI4 (Tile) for Wii
        TileCI4,

        // Special

        /// Big Endian DXT5
        DXT5,
        /// ASTC with block 4x4 for nx
        ASTC_8BPP,
        /// Big Endian BC7 compressed RGBA8 for nx
        BC7,
        /// DXT1
        DXT1);

}; // namespace PSB