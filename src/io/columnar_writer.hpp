#pragma once

#include <bit>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "src/storage/chunk_info.hpp"
#include "src/storage/conversion_batch.hpp"
#include "src/storage/schema.hpp"

namespace columnar {

class ColumnarWriter {
private:
    std::ofstream output_file_;
    std::vector<ChunkInfo> chunk_info_;
    const Schema& schema_;
    std::vector<char> buffer_;
    int64_t offset_ = 0;

public:
    static ColumnarWriter open_output(const std::string& path_to_output, const Schema& schema);

    void write_batch(const ConversionBatch& batch);

    void write_metadata();

private:
    ColumnarWriter(std::ofstream&& file, const Schema& schema);

    void flush();

    template <typename T>
    void write_column(const T& column) {
        size_t bytes = column.size() * sizeof(typename T::value_type);
        if (buffer_.size() + bytes >= buffer_.capacity()) {
            flush();
        }
        const char* ptr = reinterpret_cast<const char*>(column.data());
        buffer_.insert(buffer_.end(), ptr, ptr + bytes);
        offset_ += static_cast<int64_t>(bytes);
    }

    template <typename T>
    void write_int(T value) {
        if (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }

        if (buffer_.size() + sizeof(value) >= buffer_.capacity()) {
            flush();
        }

        const char* ptr = reinterpret_cast<const char*>(&value);
        buffer_.insert(buffer_.end(), ptr, ptr + sizeof(T));

        offset_ += sizeof(value);
    }

    void write_string(const std::string& str, int64_t len);
};

}  // namespace columnar
