#pragma once

#include <cstddef>
#include <cstring>
#include <unistd.h>
#include <vector>

#include "src/storage/batch.hpp"
#include "src/storage/chunk_info.hpp"
#include "src/storage/schema.hpp"

namespace columnar {

class ColumnarReader {
private:
    int fd_;
    std::vector<char> chunk_buffer_;
    std::vector<char> meta_buffer_;
    const char* cur_ = nullptr;
    Schema schema_;
    std::vector<ChunkInfo> chunks_;
    int32_t column_count_ = 0;
    size_t current_row_group_index_ = 0;

public:
    static ColumnarReader open_output(const std::string& path);

    ~ColumnarReader();

    ColumnarReader(const ColumnarReader&) = delete;

    ColumnarReader(ColumnarReader&& other) noexcept;
    
    ColumnarReader& operator=(const ColumnarReader&) = delete;

    ColumnarReader& operator=(ColumnarReader&& other) noexcept;

    void fill_batch(Batch& batch);  // throws

    const Schema& schema() const;

private:
    ColumnarReader(int fd);

    void read_metadata();  // throws

    template <typename T>
    T read_int() {

        T value;

        std::memcpy(&value, cur_, sizeof(T));
        cur_ += sizeof(T);

        return value;
    }

    std::string read_string();  // throws

    int64_t get_meta_start() const;

    int64_t get_meta_size() const;
};

}  // namespace columnar
