#pragma once

#include <cstdint>
#include "tjs.h"

namespace PSB {
    struct PSBHeader {
        char signature[4];
        std::uint16_t version;
        std::uint16_t encrypt;
        std::uint32_t offsetEncrypt;
        std::uint32_t offsetNames;
        std::uint32_t offsetStrings;
        std::uint32_t offsetStringsData;
        std::uint32_t offsetChunkOffsets;
        std::uint32_t offsetChunkLengths;
        std::uint32_t offsetChunkData;
        std::uint32_t offsetEntries;
        std::uint32_t checksum;
        std::uint32_t offsetExtraChunkOffsets;
        std::uint32_t offsetExtraChunkLengths;
        std::uint32_t offsetExtraChunkData;

        static constexpr int MAX_LENGTH = 56;

        [[nodiscard]] bool isEncrypted() const {
            return offsetEncrypt > MAX_LENGTH + 16 || offsetNames == 0 ||
                (version > 1 && offsetEncrypt != offsetNames &&
                 offsetEncrypt != 0);
        }

        static std::uint32_t GetHeaderLength(std::uint16_t version) {
            if(version < 3)
                return 40u;
            if(version > 3)
                return 56u;
            return 44u;
        }

        [[nodiscard]] std::uint32_t GetHeaderLength() const {
            return GetHeaderLength(version);
        }
    };

    static constexpr char PsbSignature[]{ 'P', 'S', 'B', '\0' };
    static constexpr char MdfSignature[]{ 'M', 'D', 'F', '\0' };
    static constexpr char MflSignature[]{ 'M', 'F', 'L', '\0' };

    static PSBHeader parsePSBHeader(TJS::tTJSBinaryStream *stream) {
        PSBHeader header = {};

        stream->ReadBuffer(header.signature, 4);
        stream->ReadBuffer(&header.version, 2);
        stream->ReadBuffer(&header.encrypt, 2);
        stream->ReadBuffer(&header.offsetEncrypt, 4);
        stream->ReadBuffer(&header.offsetNames, 4);

        if(std::strcmp(header.signature, MdfSignature) == 0 ||
           std::strcmp(header.signature, MflSignature) == 0) {
            throw std::runtime_error("Maybe a MDF file");
        }
        if(std::strcmp(header.signature, PsbSignature) != 0) {
            throw std::runtime_error("Not a valid PSB file");
        }
        if(header.offsetNames < stream->GetSize()) {
            stream->ReadBuffer(&header.offsetStrings, 4);
            stream->ReadBuffer(&header.offsetStringsData, 4);
            stream->ReadBuffer(&header.offsetChunkOffsets, 4);
            stream->ReadBuffer(&header.offsetChunkLengths, 4);
            stream->ReadBuffer(&header.offsetChunkData, 4);
            stream->ReadBuffer(&header.offsetEntries, 4);

            if(header.version > 2) {
                stream->ReadBuffer(&header.checksum, 4);
            }
            if(header.version > 3) {
                stream->ReadBuffer(&header.offsetExtraChunkOffsets, 4);
                stream->ReadBuffer(&header.offsetExtraChunkLengths, 4);
                stream->ReadBuffer(&header.offsetExtraChunkData, 4);
            }
        }
        return header;
    }

} // namespace PSB
