#include "batch.hpp"
#include "data_type.hpp"

namespace columnar {

Batch::Batch() {
}

Batch::Batch(const Schema& schema, const std::vector<std::string>& columns_needed) : schema_(schema) {
    std::vector<std::string> new_names = columns_needed;
    if (columns_needed.size() == 1 && columns_needed[0] == "") {
        new_names.clear();
    }
    if (columns_needed.size() == 1 && columns_needed[0] == "*") {
        new_names = schema_.get_column_names();
    }
    for (const auto& name : new_names) {
        size_t idx = schema_.get_column_index(name);
        Type type = schema_.get_column_type(idx);
        columns_.emplace_back(type, idx);
        idx_to_pos_[idx] = column_count_;
        column_count_++;
    }
}

Batch::Batch(const Batch& other)
    : schema_(other.schema_), idx_to_pos_(other.idx_to_pos_), active_rows_(other.active_rows_),
      column_count_(other.column_count_), row_count_(other.row_count_) {

    columns_.reserve(other.columns_.size());

    for (const auto& col : other.columns_) {
        columns_.emplace_back(col);
    }
}

Batch::Batch(Batch&& other) noexcept {
    schema_ = other.schema_;
    columns_ = std::move(other.columns_);
    column_count_ = other.column_count_;
    row_count_ = other.row_count_;
    idx_to_pos_ = other.idx_to_pos_;
    active_rows_ = other.active_rows_;

    other.column_count_ = 0;
    other.row_count_ = 0;
    other.idx_to_pos_.clear();
}

Batch& Batch::operator=(Batch&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    schema_ = other.schema_;
    columns_ = std::move(other.columns_);
    column_count_ = other.column_count_;
    row_count_ = other.row_count_;
    idx_to_pos_ = other.idx_to_pos_;

    active_rows_ = other.active_rows_;

    other.column_count_ = 0;
    other.row_count_ = 0;

    return *this;
}

const Column& Batch::get_column_by_idx(size_t idx) const {
    ASS(idx_to_pos_.contains(idx), "no such index in batch");
    return columns_[idx_to_pos_.at(idx)];
}

void Batch::clear() {
    row_count_ = 0;
    for (size_t i = 0; i < column_count_; ++i) {
        columns_[i].clear();
    }
}

}  // namespace columnar
