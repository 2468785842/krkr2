/**
 * PSBFile header memory view
 *
 * 50 53 42 00   03 00 00 00   2c 00 00 00   2c 00 00 00   │ PSB·····,···,··· │
 * 05 09 00 00   48 09 00 00   f4 0a 00 00   07 0b 00 00   │ ····H··········· │
 * 1c 0b 00 00   d0 01 00 00   c0 02 15 27   0d c3 0d 01   │ ···········'···· │
 * 00 01 02 03   04 05 06 07   08 09 0a 0b   0c 0d 0e 0f   │ ················ │
 * 10 11 12 13   14 00 00 00   00 00 00 00   00 00 00 00   │ ················ │
 * 00 00 00 00   00 00 00 00   00 00 00 00   00 00 02 05   │ ················ │
 * 02 03 01 12   01 02 01 06   0b 02 0e 06   10 07 12 09   │ ················ │
 * 0c 0c 13 0f   16 19 0f 1b   12 00 00 00   00 00 00 00   │ ················ │
 * 00 00 00 00   00 00 00 00   00 00 00 00   00 00 00 21   │ ···············! │
 */
#ifndef KRKR2_PSBHEADER_H
#define KRKR2_PSBHEADER_H
#include <cstdint>
#include <array>

#include "tjs.h"
namespace PSB {

    struct PSBHeader {
        char signature[4];
        std::uint16_t version;
        std::uint16_t encrypt;
        std::uint32_t length;
        std::uint32_t offsetNames;
        std::uint32_t offsetStrings;
        std::uint32_t offsetStringsData;
        std::uint32_t offsetChunkLengths;
        std::uint32_t offsetChunkData;
        std::uint32_t offsetEntries;
        std::uint32_t checksum;
    };

    static constexpr std::array<char, 4> PsbSignature = { 'P', 'S', 'B', '\0' };

    static bool parsePSBHeader(void *buffer, PSBHeader *psbHeader) {
        if(!buffer)
            return false;
        std::memcpy(psbHeader, buffer, sizeof(PSBHeader));
        return true;
    }


} // namespace PSB
#endif // KRKR2_PSBHEADER_H
