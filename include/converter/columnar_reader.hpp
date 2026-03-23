#pragma once

#include "batch.hpp"
#include "chunk_info.hpp"
#include "parse_error.hpp"
#include "schema.hpp"

#include <expected>
#include <fstream>
#include <vector>

namespace columnar {

class ColumnarReader {
private:
    std::ifstream output_file_{};
    Schema schema_{};
    std::vector<ChunkInfo> chunks_{};
    int64_t column_count_{};
    size_t current_chunk_index_{};

public:
    static Expected<ColumnarReader> open_output(const std::string& path);

    Expected<bool> fill_batch(Batch& batch);

    const Schema& schema() const;

private:
    ColumnarReader(std::ifstream&& output_file);

    Expected<void> read_metadata();

    int64_t read_int64();
};

}  // namespace columnar
