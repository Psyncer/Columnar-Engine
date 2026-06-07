#pragma once

#include <unordered_map>
#include <vector>

#include "src/storage/column.hpp"
#include "src/storage/schema.hpp"

namespace columnar {

struct Batch {
    Schema schema_;
    std::vector<Column> columns_;
    std::unordered_map<size_t, size_t> idx_to_pos_;
    std::unordered_map<size_t, size_t> pos_to_idx_;
    std::vector<size_t> active_rows_;
    size_t column_count_ = 0;
    size_t row_count_ = 0;

    Batch();

    Batch(const Schema& schema, const std::vector<std::string>& columns_needed);

    Batch(const Batch& other);

    Batch& operator=(const Batch& other) = delete;

    Batch(Batch&& other) noexcept;

    Batch& operator=(Batch&& other) noexcept;

    const Column& get_column_by_idx(size_t idx) const;

    size_t get_column_index(size_t idx) const;

    void clear();
};

}  // namespace columnar
