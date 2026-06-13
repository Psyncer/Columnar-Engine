#include <bit>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

#include "tools/assert.hpp"
#include "src/storage/batch.hpp"
#include "src/storage/chunk_info.hpp"
#include "src/io/columnar_reader.hpp"
#include "src/storage/data_type.hpp"
#include "src/storage/schema.hpp"

namespace columnar {

ColumnarReader ColumnarReader::open_output(const std::string& path) {
    ASS(std::endian::native == std::endian::little, "ggs fellas, byteswapping needed");

    int fd = open(path.c_str(), O_RDONLY);
    ASS(fd != -1, "file not found");

    ColumnarReader reader = ColumnarReader(fd);
    reader.read_metadata();

    return reader;
}

ColumnarReader::ColumnarReader(int fd) : fd_(fd), chunk_buffer_(kTempBufferSize) {
}

ColumnarReader::ColumnarReader(ColumnarReader&& other) noexcept {
    fd_ = other.fd_;
    other.fd_ = -1;
    chunk_buffer_ = std::move(other.chunk_buffer_);
    meta_buffer_ = std::move(other.meta_buffer_);
    cur_ = other.cur_;
    other.cur_ = nullptr;
    schema_ = other.schema_;
    chunks_ = other.chunks_;
    column_count_ = other.column_count_;
    current_row_group_index_ = other.current_row_group_index_;
}

ColumnarReader& ColumnarReader::operator=(ColumnarReader&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    int n = close(fd_);
    ASS(n != -1, "bad close");

    fd_ = other.fd_;
    other.fd_ = -1;
    chunk_buffer_ = std::move(other.chunk_buffer_);
    meta_buffer_ = std::move(other.meta_buffer_);
    cur_ = other.cur_;
    other.cur_ = nullptr;
    schema_ = other.schema_;
    chunks_ = other.chunks_;
    column_count_ = other.column_count_;
    current_row_group_index_ = other.current_row_group_index_;

    return *this;
}

ColumnarReader::~ColumnarReader() {
    if (fd_ != -1) {
        int n = close(fd_);
        ASS(n != -1, "bad close");
    }
}

void ColumnarReader::read_metadata() {
    int64_t meta_start = get_meta_start();
    int64_t meta_size = get_meta_size();

    meta_buffer_ = std::vector<char>(static_cast<size_t>(meta_size));
    ssize_t n = pread(fd_, meta_buffer_.data(), static_cast<size_t>(meta_size),
                      static_cast<off_t>(meta_start));
    ASS(n == meta_size, "bad pread");

    cur_ = meta_buffer_.data();

    column_count_ = read_int<int32_t>();

    for (int i = 0; i < column_count_; ++i) {
        std::string name = read_string();

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

void ColumnarReader::fill_batch(Batch& batch) {
    batch.clear();

    size_t total_columns = schema_.get_column_count();
    size_t active_columns = batch.column_count_;

    if (current_row_group_index_ * total_columns >= chunks_.size()) {
        return;
    }

    batch.row_count_ = chunks_[current_row_group_index_ * total_columns].num_rows;

    for (size_t i = 0; i < active_columns; ++i) {
        Type type = batch.columns_[i].type();
        size_t idx = static_cast<size_t>(batch.get_column_index(i));
        const ChunkInfo& chunk = chunks_[current_row_group_index_ * total_columns + idx];

        if (chunk.size_in_bytes > static_cast<int64_t>(kTempBufferSize)) {
            chunk_buffer_.resize(static_cast<size_t>(chunk.size_in_bytes));
        }
        
        ssize_t n = pread(fd_, chunk_buffer_.data(), static_cast<size_t>(chunk.size_in_bytes),
                          chunk.offset);
        ASS(n == static_cast<ssize_t>(chunk.size_in_bytes), "bad pread");

        switch (type) {
        case Type::Int16:
            batch.columns_[i].emplace_column<int16_t>(chunk_buffer_.data(),
                                                      static_cast<size_t>(chunk.size_in_bytes));
            break;
        case Type::Int32:
            batch.columns_[i].emplace_column<int32_t>(chunk_buffer_.data(),
                                                      static_cast<size_t>(chunk.size_in_bytes));
            break;
        case Type::Int64:
            batch.columns_[i].emplace_column<int64_t>(chunk_buffer_.data(),
                                                      static_cast<size_t>(chunk.size_in_bytes));
            break;
        case Type::String:
            batch.columns_[i].emplace_string_column(chunk_buffer_.data(),
                                                    static_cast<size_t>(chunk.size_in_bytes));
            break;
        case Type::Date:
            batch.columns_[i].emplace_column<int32_t>(chunk_buffer_.data(),
                                                      static_cast<size_t>(chunk.size_in_bytes));
            break;
        case Type::Timestamp:
            batch.columns_[i].emplace_column<int64_t>(chunk_buffer_.data(),
                                                      static_cast<size_t>(chunk.size_in_bytes));
            break;
        }

        batch.row_count_ = chunk.num_rows;
    }

    current_row_group_index_++;
}

const Schema& ColumnarReader::schema() const {
    return schema_;
}

std::string ColumnarReader::read_string() {
    int32_t len = read_int<int32_t>();
    std::string str;
    str.resize(static_cast<size_t>(len));

    std::memcpy(str.data(), cur_, static_cast<size_t>(len));
    cur_ += len;

    return str;
}

int64_t ColumnarReader::get_meta_start() const {
    off_t n = lseek(fd_, -8, SEEK_END);
    ASS(n != -1, "bad lseek");

    int64_t meta_start;

    n = read(fd_, &meta_start, 8);
    ASS(n == 8, "bad read");

    return meta_start;
}

int64_t ColumnarReader::get_meta_size() const {
    off_t n = lseek(fd_, -16, SEEK_END);
    ASS(n != -1, "bad lseek");

    int64_t meta_size;

    n = read(fd_, &meta_size, 8);
    ASS(n == 8, "bad read");

    return meta_size;
}

}  // namespace columnar
