#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "data_type.hpp"
#include "expression.hpp"

namespace columnar {

ColumnRef::ColumnRef(std::string name) : name_(std::move(name)) {
}

Column ColumnRef::evaluate(const Batch& batch) {
    size_t idx = batch.schema_.get_column_index(name_);
    Column column(batch.columns_[batch.idx_to_pos_.at(idx)]);

    return column;
}

Column Literal::evaluate([[maybe_unused]] const Batch& batch) {
    if (std::holds_alternative<std::string>(literal_)) {
        return Column(Type::String, 0, std::get<std::string>(literal_));
    }

    return Column(Type::Int64, 0, std::get<int64_t>(literal_));
}

NotEqual::NotEqual(std::unique_ptr<ColumnRef>&& left, std::unique_ptr<Literal>&& right)
    : left_(std::move(left)), right_(std::move(right)) {
}

void NotEqual::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    Column left(left_->evaluate(batch));
    Column right(right_->evaluate(batch));

    // ASS same type for left and right

    switch (left.type()) {
    case Type::Int16:
        dispatch_int_comparator<int16_t>(left, right, mask);
        break;
    case Type::Int32:
        dispatch_int_comparator<int32_t>(left, right, mask);
        break;
    case Type::Int64:
        dispatch_int_comparator<int64_t>(left, right, mask);
        break;
    case Type::String:
        dispatch_string_comparator(left, right, mask);
        break;
    case Type::Date:
        dispatch_int_comparator<int32_t>(left, right, mask);
        break;
    case Type::Timestamp:
        dispatch_int_comparator<int64_t>(left, right, mask);
        break;
    }
}

void NotEqual::dispatch_string_comparator(const Column& left, const Column& right,
                                          std::vector<uint8_t>& mask) {
    for (size_t i = 0; i < left.size(); ++i) {
        if (left.get_string(i) == right.get_string(i)) {
            mask[i] = 0;
        }
    }
}

}  // namespace columnar
