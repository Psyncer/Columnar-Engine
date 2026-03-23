#include "batch.hpp"
#include "data_type.hpp"
#include "parse_error.hpp"
#include "parsing.hpp"
#include "schema.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <utility>
#include <vector>

namespace columnar {

Batch::Batch(const Schema& schema, size_t capacity) : schema_(schema), capacity_(capacity) {
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        auto column = schema_.get_column(i);
        if (!column.has_value()) {
            std::unexpected(column.error());
        }

        if (column->type_ == DataType::int64) {
            std::vector<int64_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
        } else if (column->type_ == DataType::string) {
            std::vector<std::string> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
        }
    }
}

Expected<void> Batch::add_row(const std::vector<std::string>& row) {
    for (size_t i = 0; i < row.size(); ++i) {
        DataType type = schema_.get_column(i)->type_;

        if (type == DataType::int64) {
            auto value = parse_int64(row[i]);
            if (!value.has_value()) {
                return std::unexpected(value.error());
            }

            auto res = columns_[i].append<int64_t>(*value);
            if (!res.has_value()) {
                return std::unexpected(res.error());
            }
        } else if (type == DataType::string) {
            auto res = columns_[i].append<std::string>(row[i]);
            if (!res.has_value()) {
                return std::unexpected(res.error());
            }
        } else {
            return std::unexpected(parse_error::not_implemented);
        }
    }

    row_count_++;

    return {};
}

Expected<const BatchColumn&> Batch::get_column(size_t idx) const {
    return columns_[idx];
}

size_t Batch::get_row_count() const {
    return row_count_;
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

}  // namespace columnar
