//
// Created by lidong on 25-3-18.
//
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace BitConverter {
    template <typename T>
    std::vector<std::uint8_t> toByteArray(T value) {
        std::vector<std::uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        return bytes;
    }

    template <typename T>
    T fromByteArray(const std::vector<std::uint8_t> &bytes) {
        T value;
        std::memcpy(&value, bytes.data(), sizeof(T));
        return value;
    }
} // namespace BitConverter