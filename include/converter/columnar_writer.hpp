#pragma once

#include <bit>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "batch.hpp"
#include "chunk_info.hpp"
#include "schema.hpp"

namespace columnar {

class ColumnarWriter {
private:
    std::ofstream output_file_;
    std::vector<ChunkInfo> chunk_info_;
    const Schema& schema_;
    int64_t offset_ = 0;

public:
    static ColumnarWriter open_output(const std::string& path_to_output, const Schema& schema);

    void write_batch(const Batch& batch);  // throws

    void write_metadata();

private:
    ColumnarWriter(std::ofstream&& file, const Schema& schema);

    template <typename T>
    void write_int(T value) {
        if (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }

        output_file_.write(reinterpret_cast<char*>(&value), sizeof(value));  // throws
        offset_ += sizeof(value);
    }

    void write_string(const std::string& str, int64_t len);  // throws

    void write_char(char ch);  // throws
};

}  // namespace columnar
