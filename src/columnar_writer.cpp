#include "columnar_writer.hpp"
#include "batch.hpp"
#include "chunk_info.hpp"
// #include "parse_error.hpp"
#include "schema.hpp"

#include <bit>
#include <cstdint>
// #include <expected>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

namespace columnar {

void ColumnarWriter::write_int64(int64_t value) {
    if (std::endian::native == std::endian::big) {
        value = std::byteswap(value);
    }

    file_.write(reinterpret_cast<char*>(&value), sizeof(value));
    offset_ += 8;
}

ColumnarWriter ColumnarWriter::open_columnar_to_write(const std::string& path,
                                                      const Schema& schema) {
    std::ofstream file(path, std::ios::binary);
    ColumnarWriter writer(std::move(file), schema);

    return writer;
}

void ColumnarWriter::write_batch(const Batch& batch) {
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        ChunkInfo chunk;
        chunk.offset = offset_;
        chunk.column_index = i;
        chunk.num_rows = batch.get_row_count();

        const BatchColumn& column_data = batch.get_column(i);
        DataType type = schema_.get_column(i).type_;

        if (type == DataType::int64) {
            const auto& column = column_data.get<int64_t>();

            for (const auto& value : column) {
                write_int64(value);
            }

        } else if (type == DataType::string) {
            const auto& column = column_data.get<std::string>();

            for (const auto& str : column) {
                int64_t len = static_cast<int64_t>(str.size());
                write_int64(len);
                file_.write(str.data(), static_cast<std::streamsize>(len));
                offset_ += len;
            }
        }
        chunk.size_in_bytes = offset_ - chunk.offset;
        chunk_info_.push_back(chunk);
    }
}

void ColumnarWriter::write_metadata() {
    int64_t metastart = offset_;
    write_int64(static_cast<int64_t>(schema_.get_column_count()));
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        auto column = schema_.get_column(i);
        int64_t len = static_cast<int64_t>(column.name_.size());
        write_int64(len);
        file_.write(column.name_.data(), static_cast<std::streamsize>(len));
        offset_ += len;
        int64_t type = static_cast<int64_t>(column.type_);  // enum
        write_int64(type);
    }
    int64_t num_chunks = static_cast<int64_t>(chunk_info_.size());
    write_int64(num_chunks);
    for (size_t i = 0; i < static_cast<size_t>(num_chunks); ++i) {
        write_int64(static_cast<int64_t>(chunk_info_[i].column_index));
        write_int64(static_cast<int64_t>(chunk_info_[i].offset));
        write_int64(static_cast<int64_t>(chunk_info_[i].size_in_bytes));
        write_int64(static_cast<int64_t>(chunk_info_[i].num_rows));
    }
    int64_t metasize = offset_ - metastart;
    write_int64(metasize);
}

ColumnarWriter::ColumnarWriter(std::ofstream file, const Schema& schema)
    : file_(std::move(file)), chunk_info_(0), schema_(schema), offset_(0) {
}

}  // namespace columnar
