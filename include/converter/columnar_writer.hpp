#pragma once

#include "batch.hpp"
#include "chunk_info.hpp"
#include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
#include <expected>
#include <fstream>
#include <string>
#include <vector>

namespace columnar {

class ColumnarWriter {
private:
    std::ofstream output_file_{};
    std::vector<ChunkInfo> chunk_info_{};
    const Schema& schema_{};
    int64_t offset_{};
    
public:
    static Expected<ColumnarWriter> open_output(const std::string& path_to_output,
                                                const Schema& schema);

    Expected<void> write_batch(const Batch& batch);

    Expected<void> write_metadata();

private:
    ColumnarWriter(std::ofstream&& file, const Schema& schema);

    void write_int64(int64_t value);
};

}  // namespace columnar
