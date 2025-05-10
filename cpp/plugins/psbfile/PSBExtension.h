//
// Created by lidong on 25-3-18.
//
#pragma once

#include <cstdint>
#include <vector>
#include "tjs.h"

// 定义 uint 类型
#if defined(_MSC_VER)
    // 如果是 MSVC，定义 uint 为 unsigned int
    typedef unsigned int uint;
#else
    // 对于其他编译器，使用 std::uint32_t
    using uint = std::uint32_t;
#endif

namespace PSB::Extension {

    static int getSize(uint i) {
        int n = 0;
        do {
            i >>= 8;
            n++;
        } while(i != 0);
        return n;
    }

    static void readAndUnzip(TJS::tTJSBinaryStream *stream, std::uint8_t size,
                             std::vector<std::uint8_t> data,
                             bool usigned = false) {
        stream->Read(data.data(), size);

        std::uint8_t fill = 0x0;
        if(!usigned && data[size - 1] >= 0b10000000) { // negative
            fill = 0xFF;
        }

        for(int i = 0; i < data.size(); i++) {
            data[i] = i < size ? data[i] : fill;
        }
    }

    static std::string readStringZeroTrim(TJS::tTJSBinaryStream *stream) {
        std::string name;

        while(true) {
            constexpr size_t CHUNK_SIZE = 256;
            char temp[CHUNK_SIZE];
            const size_t bytes_read = stream->Read(temp, CHUNK_SIZE);
            if(bytes_read == 0)
                break;

            for(size_t j = 0; j < bytes_read; ++j) {
                if(temp[j] == '\0' || std::isspace(temp[j])) {
                    name.append(temp, j);
                    return std::move(name);
                }
            }
            name.append(temp, bytes_read);
        }

        return "";
    }
} // namespace PSB::Extension