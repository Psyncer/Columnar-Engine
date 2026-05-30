#pragma once

#include <cstddef>
#include <cstdint>

namespace columnar {

struct ChunkInfo {
    size_t column_index;
    int64_t offset;
    int64_t size_in_bytes;
    size_t num_rows;
};

}  // namespace columnar
