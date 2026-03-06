#include "batch.hpp"
#include "data_type.hpp"
// #include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
// #include <expected>
#include <string>
#include <utility>
#include <vector>

namespace columnar {

Batch::Batch(const Schema& schema, size_t capacity)
    : schema_(schema), capacity_(capacity), row_count_(0) {
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        const SchemaColumn& column = schema_.get_column(i);

        if (column.type_ == DataType::int64) {
            std::vector<int64_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
        } else if (column.type_ == DataType::string) {
            std::vector<std::string> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
        }
    }
}

void Batch::add_row(const std::vector<std::string>& row) {
    for (size_t i = 0; i < row.size(); ++i) {
        DataType type = schema_.get_column(i).type_;

        if (type == DataType::int64) {
            auto value = parse_int64(row[i]);
            columns_[i].append<int64_t>(value);  // может выкинуть
        } else if (type == DataType::string) {
            columns_[i].append<std::string>(row[i]);  // может выкинуть
        }
    }
    row_count_++;
}

void Batch::add_column(const BatchColumn& column) {
    columns_.push_back(column);
    row_count_ = capacity();
}

const BatchColumn& Batch::get_column(size_t idx) const {
    return columns_[idx];
}

const Schema& Batch::schema() const {
    return schema_;
}

size_t Batch::get_row_count() const {
    return row_count_;
}

size_t Batch::capacity() const {
    return capacity_;
}

bool Batch::is_full() const {
    return row_count_ == capacity_;
}

void Batch::clear() {
    row_count_ = 0;
    for (auto& column : columns_) {
        std::visit(
            [](auto& vec) {
                vec.clear();
            },
            column.data);
    }
}

int64_t Batch::parse_int64(const std::string& token) {
    int64_t value;
    value = std::stol(token);  // может выкинуть

    return value;
}

}  // namespace columnar
