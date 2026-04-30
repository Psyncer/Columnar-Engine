#pragma once

#include <bit>
#include <fstream>
#include <vector>

#include "batch.hpp"
#include "chunk_info.hpp"
#include "schema.hpp"

namespace columnar {

class ColumnarReader {
private:
    std::ifstream output_file_;
    Schema schema_;
    std::vector<ChunkInfo> chunks_;
    int64_t column_count_ = 0;
    size_t current_chunk_index_ = 0;

public:
    static ColumnarReader open_output(const std::string& path);

    bool fill_batch(Batch& batch);  // throws

    const Schema& schema() const;

private:
    ColumnarReader(std::ifstream&& output_file);

    void read_metadata();  // throws

    template <typename T>
    T read_int() {
        T value;
        output_file_.read(reinterpret_cast<char*>(&value), sizeof(T));  // throws

        if (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }

        return value;
    }

    std::string read_string();  // throws

    char read_char();  // throws
};

}  // namespace columnar
