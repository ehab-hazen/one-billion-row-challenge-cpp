#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

struct Data {
    float min = std::numeric_limits<float>::max(),
          max = std::numeric_limits<float>::min(), sum = 0;
    uint32_t occurences = 0;

    void operator+=(const Data& rhs) {
        min = std::min(min, rhs.min);
        max = std::max(max, rhs.max);
        sum += rhs.sum;
        occurences += rhs.occurences;
    }

    void operator+=(const float val) {
        min = std::min(min, val);
        max = std::max(max, val);
        sum += val;
        ++occurences;
    }
};
