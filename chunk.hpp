#pragma once

#include <cstddef>

struct Chunk {
    const char * data;
    size_t size;

    bool operator==(const Chunk& rhs) const {
        return data == rhs.data && size == rhs.size;
    }
};
