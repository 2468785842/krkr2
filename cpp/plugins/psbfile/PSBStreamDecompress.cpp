#include "PSBStreamDecompress.h"

#include "UtilStreams.h"

#include <cstring>
#include <memory>
#include <limits>

#include <zlib.h>

#include "PSBLz4Stream.h"

namespace PSB {
    namespace {

        std::unique_ptr<tTVPMemoryStream>
        decompressMdfStream(TJS::tTJSBinaryStream *orgStream, size_t readSize) {
            uLongf uncompressedSize = orgStream->ReadI32LE();
            std::vector<tjs_uint8> uncompressed(uncompressedSize);
            std::vector<tjs_uint8> compressed(readSize - 8);
            orgStream->ReadBuffer(compressed.data(), readSize - 8);
            if(uncompress(uncompressed.data(), &uncompressedSize,
                          compressed.data(),
                          static_cast<uLong>(readSize - 8)) != Z_OK) {
                return nullptr;
            }
            auto stream = std::make_unique<tTVPMemoryStream>(
                nullptr, static_cast<tjs_uint>(uncompressedSize));
            std::memcpy(stream->GetInternalBuffer(), uncompressed.data(),
                        uncompressedSize);
            stream->SetPosition(0);
            return stream;
        }

    } // namespace

    std::unique_ptr<tTVPMemoryStream>
    openDecompressedStream(TJS::tTJSBinaryStream *input) {
        const auto readSize = input->GetSize();
        if(readSize < 9 || readSize > std::numeric_limits<tjs_uint>::max()) {
            return nullptr;
        }

        input->SetPosition(0);
        char sign[5]{};
        input->ReadBuffer(sign, 5);
        sign[4] = '\0';
        const char lzfs[] = { 0x04, 0x22, 0x4d, 0x18, 0x00 };
        if(std::memcmp(sign, lzfs, 5) == 0) {
            input->SetPosition(0);
            return decompressLz4Stream(input);
        }

        sign[3] = '\0';
        if(std::memcmp(sign, "MDF", 3) == 0 ||
           std::memcmp(sign, "mdf", 3) == 0) {
            input->SetPosition(4);
            return decompressMdfStream(input, readSize);
        }

        auto stream = std::make_unique<tTVPMemoryStream>(
            nullptr, static_cast<tjs_uint>(readSize));
        input->SetPosition(0);
        input->ReadBuffer(stream->GetInternalBuffer(), readSize);
        stream->SetPosition(0);
        return stream;
    }

} // namespace PSB
