#pragma once

#include "batch.hpp"
#include "chunk_info.hpp"
#include "schema.hpp"

#include <fstream>
#include <vector>

namespace columnar {

class ColumnarReader {
public:
    static ColumnarReader open_columnar_to_read(const std::string& path);

    bool fill_batch(Batch& batch);

    const Schema& schema() const;

private:
    std::ifstream file_;
    Schema schema_;
    std::vector<ChunkInfo> chunks_;
    int64_t column_count_;
    size_t current_chunk_index_;

    ColumnarReader(std::ifstream file);

    void read_metadata();

    int64_t read_int64();
};

}  // namespace columnar
