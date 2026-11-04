#pragma once

#include <cstdint>

namespace krkr::input {
    struct PointerPosition {
        float x, y;
    };

    struct PointerPixel {
        std::int32_t x, y;
    };

    // Continuous coordinates: flip at height, not at height - 1. The same
    // operation converts in either direction. No clipping or DPI conversion.
    PointerPosition flipPointerY(PointerPosition position, float height);

    // Keep legacy truncation toward zero, including captured positions outside
    // the layer. Reject non-finite/out-of-range input before any integer cast.
    PointerPixel pointerPixel(PointerPosition position);
} // namespace krkr::input
