#include "columnar_reader.hpp"
#include "batch.hpp"
#include "chunk_info.hpp"
#include "csv_utils.hpp"
#include "data_type.hpp"
#include "schema.hpp"

#include <bit>
#include <fstream>
#include <string>

namespace columnar {

int64_t ColumnarReader::read_int64() {
    int64_t value;
    file_.read(reinterpret_cast<char*>(&value), 8);

    if (std::endian::native == std::endian::big) {
        value = std::byteswap(value);
    }

    return value;
}

ColumnarReader ColumnarReader::open_columnar_to_read(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    ColumnarReader reader = ColumnarReader(std::move(file));
    reader.read_metadata();

    return reader;
}

ColumnarReader::ColumnarReader(std::ifstream file)
    : file_(std::move(file)), schema_(), column_count_(), current_chunk_index_() {
}

void ColumnarReader::read_metadata() {
    file_.seekg(-8, std::ios::end);
    int64_t metastart = read_int64();

    file_.seekg(-(metastart + 8), std::ios::end);
    column_count_ = read_int64();

    for (int i = 0; i < column_count_; ++i) {
        std::string name;
        int64_t s = read_int64();

        name.resize(static_cast<size_t>(s));
        file_.read(name.data(), s);

        int64_t t = read_int64();
        DataType type;
        switch (t) {
        case 0:
            type = DataType::int64;
            break;
        case 1:
            type = DataType::string;
            break;
        }

        schema_.add_column(name, type);
    }

    int64_t num_chunks = read_int64();
    chunks_.reserve(static_cast<size_t>(num_chunks));

    for (int i = 0; i < num_chunks; ++i) {
        ChunkInfo chunk;
        chunk.column_index = static_cast<size_t>(read_int64());
        chunk.offset = read_int64();
        chunk.size_in_bytes = read_int64();
        chunk.num_rows = static_cast<size_t>(read_int64());
        chunks_.push_back(chunk);
    }
}

bool ColumnarReader::fill_batch(Batch& batch) {
    size_t num_columns = schema_.get_column_count();
    
    if (current_chunk_index_ * num_columns >= chunks_.size()) {
        return false;
    }
    
    std::vector<std::vector<std::string>> columns_data(num_columns);
    size_t num_rows = 0;
    
    for (size_t col = 0; col < num_columns; ++col) {
        size_t chunk_idx = current_chunk_index_ * num_columns + col;
        const ChunkInfo& chunk = chunks_[chunk_idx];
        file_.seekg(chunk.offset);    
        num_rows = chunk.num_rows;
        columns_data[col].reserve(num_rows);
        DataType type = schema_.get_column(col).type_;
        
        if (type == DataType::int64) {
            for (size_t row = 0; row < num_rows; ++row) {
                int64_t value = read_int64();
                columns_data[col].push_back(std::to_string(value));
            }
        } else if (type == DataType::string) {
            for (size_t row = 0; row < num_rows; ++row) {
                int64_t len = read_int64();
                std::string str;
                str.resize(static_cast<size_t>(len));
                file_.read(str.data(), len);
                columns_data[col].push_back(str);
            }
        }
    }
    
    for (size_t row = 0; row < num_rows; ++row) {
        std::vector<std::string> row_data;
        row_data.reserve(num_columns);
        
        for (size_t col = 0; col < num_columns; ++col) {
            row_data.push_back(columns_data[col][row]);
        }
        
        batch.add_row(row_data);
    }
    
    current_chunk_index_++;
    
    return current_chunk_index_ * num_columns < chunks_.size();
}

const Schema& ColumnarReader::schema() const {
    return schema_;
}

}  // namespace columnar
