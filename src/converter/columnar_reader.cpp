#include <fstream>
#include <iostream>
#include <string>

#include "assert.hpp"
#include "batch.hpp"
#include "chunk_info.hpp"
#include "columnar_reader.hpp"
#include "data_type.hpp"
#include "schema.hpp"

namespace columnar {

ColumnarReader ColumnarReader::open_output(const std::string& path) {
    std::ifstream output_file(path, std::ios::binary);
    ASS(output_file.is_open(), "file not found");

    ColumnarReader reader = ColumnarReader(std::move(output_file));
    reader.read_metadata();

    return reader;
}

ColumnarReader::ColumnarReader(std::ifstream&& output_file) : output_file_(std::move(output_file)) {
}

void ColumnarReader::read_metadata() {
    output_file_.seekg(-8, std::ios::end);
    int64_t metastart = read_int<int64_t>();

    output_file_.seekg(-(metastart + 8), std::ios::end);
    column_count_ = read_int<int32_t>();

    for (int i = 0; i < column_count_; ++i) {
        std::string name;
        int32_t s = read_int<int32_t>();

        name.resize(static_cast<size_t>(s));
        output_file_.read(name.data(), s);

        int16_t t = read_int<int16_t>();
        Type type;
        switch (t) {
        case 0:
            type = Type::Int16;
            break;
        case 1:
            type = Type::Int32;
            break;
        case 2:
            type = Type::Int64;
            break;
        case 3:
            type = Type::String;
            break;
        case 4:
            type = Type::Date;
            break;
        case 5:
            type = Type::Timestamp;
            break;
        case 6:
            type = Type::Char;
            break;
        default:
            std::cerr << "Type not implemented" << "\n  at " << __FILE__ << ":" << __LINE__
                      << "\n  in " << __func__ << std::endl;
            std::abort();
        }

        schema_.add_column(name, type);
    }

    int64_t num_chunks = read_int<int64_t>();
    chunks_.reserve(static_cast<size_t>(num_chunks));

    for (int i = 0; i < num_chunks; ++i) {
        ChunkInfo chunk;
        chunk.column_index = static_cast<size_t>(read_int<int32_t>());
        chunk.offset = read_int<int64_t>();
        chunk.size_in_bytes = read_int<int64_t>();
        chunk.num_rows = static_cast<size_t>(read_int<int32_t>());
        chunks_.push_back(chunk);
    }
}

bool ColumnarReader::fill_batch(Batch& batch) {
    size_t num_columns = schema_.get_column_count();

    if (current_chunk_index_ * num_columns >= chunks_.size()) {
        return false;
    }

    size_t num_rows = 0;

    for (size_t col = 0; col < num_columns; ++col) {
        size_t chunk_idx = current_chunk_index_ * num_columns + col;
        const ChunkInfo& chunk = chunks_[chunk_idx];
        output_file_.seekg(chunk.offset);
        num_rows = chunk.num_rows;
        Type type = schema_.get_column(col).type_;

        switch (type) {
        case Type::Int16:
            for (size_t row = 0; row < num_rows; ++row) {
                int16_t value = read_int<int16_t>();
                batch.append_column(col, value);
            }
            break;
        case Type::Int32:
            for (size_t row = 0; row < num_rows; ++row) {
                int32_t value = read_int<int32_t>();
                batch.append_column(col, value);
            }
            break;
        case Type::Int64:
            for (size_t row = 0; row < num_rows; ++row) {
                int64_t value = read_int<int64_t>();
                batch.append_column(col, value);
            }
            break;
        case Type::String:
            for (size_t row = 0; row < num_rows; ++row) {
                std::string str = read_string();
                batch.append_column(col, str);
            }
            break;
        case Type::Date:
            for (size_t row = 0; row < num_rows; ++row) {
                int32_t value = read_int<int32_t>();
                batch.append_column(col, value);
            }
            break;
        case Type::Timestamp:
            for (size_t row = 0; row < num_rows; ++row) {
                int64_t value = read_int<int64_t>();
                batch.append_column(col, value);
            }
            break;
        case Type::Char:
            for (size_t row = 0; row < num_rows; ++row) {
                char ch = read_char();
                batch.append_column(col, ch);
            }
            break;
        }
    }

    batch.set_row_count(num_rows);
    current_chunk_index_++;

    return current_chunk_index_ * num_columns < chunks_.size();
}

const Schema& ColumnarReader::schema() const {
    return schema_;
}

std::string ColumnarReader::read_string() {
    int32_t len = read_int<int32_t>();
    std::string str;
    str.resize(static_cast<size_t>(len));
    output_file_.read(str.data(), len);

    return str;
}

char ColumnarReader::read_char() {
    char ch;
    output_file_.read(&ch, 1);

    return ch;
}

}  // namespace columnar
