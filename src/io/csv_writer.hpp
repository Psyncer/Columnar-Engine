#pragma once

#include <string>

#include "src/storage/batch.hpp"

namespace columnar {

class CsvWriter {
public:
    static void write_batch(const Batch& batch);

    static std::string convert_to_date(int32_t days);

    static std::string convert_to_timestamp(int64_t seconds);
};

}  // namespace columnar
