#include "batch.hpp"

namespace columnar {

Batch::Batch() {
}

Batch::Batch(const Schema& schema, const std::vector<std::string>& names) : schema_(schema) {
    for (const auto& name : names) {
        size_t idx = schema_.get_column_index(name);
        Type type = schema_.get_column_type(idx);
        columns_.emplace_back(type, idx);
        idx_to_pos_[idx] = column_count_;
        column_count_++;
    }
}

Batch::Batch(Batch&& other) noexcept {
    schema_ = other.schema_;
    columns_ = std::move(other.columns_);
    column_count_ = other.column_count_;
    row_count_ = other.row_count_;

    other.column_count_ = 0;
    other.row_count_ = 0;
}

Batch& Batch::operator=(Batch&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    schema_ = other.schema_;
    columns_ = std::move(other.columns_);
    column_count_ = other.column_count_;
    row_count_ = other.row_count_;

    other.column_count_ = 0;
    other.row_count_ = 0;

    return *this;
}

Column& Batch::get_column_by_idx(size_t idx) {
    return columns_[idx_to_pos_.at(idx)];
}

void Batch::clear() {
    row_count_ = 0;
    for (size_t i = 0; i < column_count_; ++i) {
        columns_[i].clear();
    }
}

}  // namespace columnar
