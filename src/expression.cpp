#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "expression.hpp"

namespace columnar {

ColumnRef::ColumnRef(std::string name) : name_(std::move(name)) {
}

const Column* ColumnRef::get_column(const Batch& batch) const {
    size_t idx = batch.schema_.get_column_index(name_);
    return &batch.columns_[batch.idx_to_pos_.at(idx)];
}

std::variant<int64_t, std::string> Literal::get_literal() {
    return literal_;
}

NotEqual::NotEqual(std::unique_ptr<ColumnRef> left, std::unique_ptr<Literal> right)
    : left_(std::move(left)), right_(std::move(right)) {
}

void NotEqual::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    const Column* left = left_->get_column(batch);
    std::variant<int64_t, std::string> right = right_->get_literal();

    int64_t value = 0;
    std::string str;
    if (std::holds_alternative<int64_t>(right)) {
        value = std::get<int64_t>(right);
    } else {
        str = std::get<std::string>(right);
    }

    switch (left->type()) {
    case Type::Int16:
        dispatch_int_comparator<int16_t>(left, static_cast<int16_t>(value), mask);
        break;
    case Type::Int32:
        dispatch_int_comparator<int32_t>(left, static_cast<int32_t>(value), mask);
        break;
    case Type::Int64:
        dispatch_int_comparator<int64_t>(left, static_cast<int64_t>(value), mask);
        break;
    case Type::String:
        dispatch_string_comparator(left, str, mask);
        break;
    case Type::Date:
        dispatch_int_comparator<int32_t>(left, static_cast<int32_t>(value), mask);
        break;
    case Type::Timestamp:
        dispatch_int_comparator<int64_t>(left, static_cast<int64_t>(value), mask);
        break;
    }
}

void NotEqual::dispatch_string_comparator(const Column* column, std::string str,
                                          std::vector<uint8_t>& mask) {
    size_t len = str.size();
    const char* data = str.data();
    for (size_t i = 0; i < column->size(); ++i) {
        if (len != column->string_len(i)) {
            continue;
        }
        if (std::memcmp(column->data_as<char>() + column->string_offset(i), data, len) == 0) {
            mask[i] = 0;
        }
    }
}

}  // namespace columnar
