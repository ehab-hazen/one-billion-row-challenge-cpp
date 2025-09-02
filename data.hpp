#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

struct Data {
    float min = std::numeric_limits<float>::max(),
          max = std::numeric_limits<float>::min(), sum = 0;
    uint32_t occurences = 0;

    Data combine(const Data &rhs) const {
        return {
            std::min(min, rhs.min),
            std::max(max, rhs.max),
            sum + rhs.sum,
            occurences + rhs.occurences,
        };
    }
};
