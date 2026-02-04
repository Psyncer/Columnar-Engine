#pragma once

#include <cstdint>

namespace columnar {

enum class parse_error : int64_t {
    file_not_found          = 0,
    invalid_schema_format   = 1,
    unsupported_type        = 2,
    empty_schema            = 3,
    duplicate_column        = 4,
    full_batch              = 5,
    invalid_row_size        = 6,
    index_out_of_range      = 7,
};

}  // namespace columnar
