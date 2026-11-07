#include "PointerCoordinates.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace krkr::input {
    PointerPosition flipPointerY(PointerPosition position, float height) {
        return { position.x, height - position.y };
    }

    PointerPixel pointerPixel(PointerPosition position) {
        const auto valid = [](float coordinate) {
            const double value = coordinate;
            return std::isfinite(value) &&
                value >= std::numeric_limits<std::int32_t>::min() &&
                value <= std::numeric_limits<std::int32_t>::max();
        };
        if(!valid(position.x) || !valid(position.y))
            throw std::out_of_range(
                "Pointer coordinates exceed the pixel range");
        return { static_cast<std::int32_t>(position.x),
                 static_cast<std::int32_t>(position.y) };
    }
} // namespace krkr::input
