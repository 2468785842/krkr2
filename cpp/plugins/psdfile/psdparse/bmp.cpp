#include <cstring>
#include <iostream>
#include <fstream>
#include "psdbase.h"

namespace psd {

#pragma pack(1)
    struct BmpFileHeader {
        uint16_t type;
        uint32_t size;
        uint16_t reserved1;
        uint16_t reserved2;
        uint32_t offBits;
    };

    struct BmpInfoHeader {
        uint32_t size;
        int32_t width;
        int32_t height;
        uint16_t planes;
        uint16_t bitCount;
        uint32_t compression;
        uint32_t sizeImage;
        int32_t xPelsPerMeter;
        int32_t yPelsPerMeter;
        uint32_t clrUsed;
        uint32_t clrImportant;
    };
#pragma pack()

    bool saveBmp(void *buffer, int width, int height, int size,
                 const char *filename) {
        BmpFileHeader bmfh;
        memset(&bmfh, 0, sizeof(BmpFileHeader));
        bmfh.type = 0x4d42;
        bmfh.size = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + size;
        bmfh.offBits = 0x36;

        BmpInfoHeader info;
        memset(&info, 0, sizeof(BmpInfoHeader));
        info.size = sizeof(BmpInfoHeader);
        info.width = width;
        info.height = height;
        info.planes = 1;
        info.bitCount = 32;
        info.compression = 0; // BI_RGB;
        info.sizeImage = 0;
        info.xPelsPerMeter = 0;
        info.yPelsPerMeter = 0;
        info.clrUsed = 0;
        info.clrImportant = 0;

        std::ofstream out;
        out.open(filename,
                 std::ofstream::out | std::ofstream::binary |
                     std::ofstream::trunc);
        if(out.is_open()) {
            out.write((char *)&bmfh, sizeof(BmpFileHeader));
            out.write((char *)&info, sizeof(BmpInfoHeader));
            out.write((char *)buffer, size);
            out.close();
        } else {
            std::cout << "could not open output bmp file: '" << filename << "'"
                      << std::endl;
            return false;
        }

        return true;
    }

} // namespace psd
