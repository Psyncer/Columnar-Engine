#pragma once

#include <unordered_map>
#include <vector>

#include "column.hpp"
#include "schema.hpp"

namespace columnar {

struct Batch {
    Schema schema_;
    std::vector<Column> columns_;
    std::unordered_map<size_t, size_t> idx_to_pos_;
    std::vector<size_t> active_rows_;
    size_t column_count_ = 0;
    size_t row_count_ = 0;

    Batch();

    Batch(const Schema& schema, const std::vector<std::string>& names);

    Batch(const Batch&) = delete;

    Batch& operator=(const Batch&) = delete;

    Batch(Batch&& other) noexcept;

    Batch& operator=(Batch&& other) noexcept;

    Column& get_column_by_idx(size_t idx);

    void clear();
};

}  // namespace columnar
