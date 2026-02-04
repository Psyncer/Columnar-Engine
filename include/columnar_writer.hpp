#pragma once

#include "batch.hpp"
#include "chunk_info.hpp"
// #include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
// #include <expected>
#include <fstream>
#include <string>
#include <vector>

namespace columnar {

class ColumnarWriter {
public:
    static ColumnarWriter open_columnar_to_write(const std::string& path, const Schema& schema);

    void write_batch(const Batch& batch);

    void write_metadata();

private:
    std::ofstream file_;
    std::vector<ChunkInfo> chunk_info_;
    const Schema& schema_;
    int64_t offset_;

    ColumnarWriter(std::ofstream file, const Schema& schema);

    void write_int64(int64_t value);
};

}  // namespace columnar
