#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "assert.hpp"
#include "chunk_info.hpp"
#include "columnar_writer.hpp"
#include "config.hpp"
#include "conversion_batch.hpp"
#include "schema.hpp"

namespace columnar {

ColumnarWriter ColumnarWriter::open_output(const std::string& path_to_output,
                                           const Schema& schema) {
    std::ofstream output_file(path_to_output, std::ios::binary);
    ASS(output_file.is_open(), "file not found");

    ColumnarWriter writer(std::move(output_file), schema);

    return writer;
}

ColumnarWriter::ColumnarWriter(std::ofstream&& file, const Schema& schema)
    : output_file_(std::move(file)), schema_(schema) {
    buffer_.reserve(kWriteBufSize);
}

void ColumnarWriter::write_batch(const ConversionBatch& batch) {
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        ChunkInfo chunk;
        chunk.offset = offset_;
        chunk.column_index = i;
        chunk.num_rows = batch.get_row_count();

        const BatchColumn& column_data = batch.get_column(i);
        Type type = schema_.get_column_type(i);

        switch (type) {
        case Type::Int16: {
            const auto& column = column_data.get<int16_t>();
            write_column(column);
            break;
        }
        case Type::Int32: {
            const auto& column = column_data.get<int32_t>();
            write_column(column);
            break;
        }
        case Type::Int64: {
            const auto& column = column_data.get<int64_t>();
            write_column(column);
            break;
        }
        case Type::String: {
            const auto& column = column_data.get<std::string>();
            for (const auto& str : column) {
                int32_t len = static_cast<int32_t>(str.size());
                write_int<int32_t>(len);
                write_string(str, len);
            }
            break;
        }
        case Type::Date: {
            const auto& column = column_data.get<int32_t>();
            write_column(column);
            break;
        }
        case Type::Timestamp: {
            const auto& column = column_data.get<int64_t>();
            write_column(column);
            break;
        }
        }

        chunk.size_in_bytes = offset_ - chunk.offset;
        chunk_info_.push_back(chunk);
    }

    flush();
}

void ColumnarWriter::write_metadata() {
    int64_t meta_start = offset_;
    write_int<int32_t>(static_cast<int32_t>(schema_.get_column_count()));

    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        const std::string& column_name = schema_.get_column_name(i);
        Type column_type = schema_.get_column_type(i);

        int32_t len = static_cast<int32_t>(column_name.size());
        int16_t type = static_cast<int16_t>(column_type);  // enum

        write_int<int32_t>(len);
        write_string(column_name, len);
        write_int<int16_t>(type);
    }

    int64_t num_chunks = static_cast<int64_t>(chunk_info_.size());
    write_int<int64_t>(num_chunks);

    for (size_t i = 0; i < static_cast<size_t>(num_chunks); ++i) {
        write_int<int32_t>(static_cast<int32_t>(chunk_info_[i].column_index));
        write_int<int64_t>(static_cast<int64_t>(chunk_info_[i].offset));
        write_int<int64_t>(static_cast<int64_t>(chunk_info_[i].size_in_bytes));
        write_int<int32_t>(static_cast<int32_t>(chunk_info_[i].num_rows));
    }

    int64_t meta_size = offset_ - meta_start;
    write_int<int64_t>(meta_size);
    write_int<int64_t>(meta_start);

    flush();
}

void ColumnarWriter::flush() {
    if (!buffer_.empty()) {
        output_file_.write(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
        buffer_.clear();
    }
}

void ColumnarWriter::write_string(const std::string& str, int64_t len) {
    if (buffer_.size() + static_cast<size_t>(len) >= buffer_.capacity()) {
        flush();
    }

    const char* ptr = str.data();
    buffer_.insert(buffer_.end(), ptr, ptr + len);

    offset_ += len;
}

}  // namespace columnar
