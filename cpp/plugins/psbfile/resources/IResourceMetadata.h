#pragma once

#include <cstdint>
#include <string>
#include "../PSBEnums.h"

namespace PSB {
    class IResourceMetadata {
    public:
        virtual ~IResourceMetadata() = default;

        // void Link(std::string fullPath, FreeMountContext context);

        [[nodiscard]] virtual std::string getName() const = 0;
        virtual void setName(std::string name) = 0;
        /**
         * Index is a value for tracking resource when compiling.
         *  For index appeared in texture name
         */
        [[nodiscard]] virtual std::uint32_t getIndex() const = 0;

        virtual void setIndex(std::uint32_t index) = 0;

        [[nodiscard]] virtual PSBSpec getSpec() const = 0;
        virtual void setSpec(PSBSpec spec) = 0;

        [[nodiscard]] virtual PSBType getPSBType() const = 0;
        virtual void setPSBType(PSBType psbType) = 0;
    };


    // Compression in PSB
    enum class PSBCompressType {
        // Normal
        None,

        // RLE
        RL,

        // Raw Bitmap
        Bmp,

        // KRKR TLG
        Tlg,

        // Astc,

        // Bc7,

        // By extension
        ByName,
    };
}; // namespace PSB