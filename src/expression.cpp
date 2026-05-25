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

const Column* ColumnRef::evaluate(const Batch& batch) {
    size_t idx = batch.schema_.get_column_index(name_);
    column_ = &batch.get_column_by_idx(idx);

    return column_;
}

std::string ColumnRef::name() const {
    return name_;
}

const Column* Literal::evaluate([[maybe_unused]] const Batch& batch) {
    if (std::holds_alternative<std::string>(literal_)) {
        column_ = Column(Type::String, 0, std::get<std::string>(literal_));
        return &column_;
    }

    column_ = Column(Type::Int64, 0, std::get<int64_t>(literal_));

    return &column_;
}

Add::Add(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right)
    : left_(std::move(left)), right_(std::move(right)), column_(Type::Int16, 0) {
}

const Column* Add::evaluate(const Batch& batch) {
    const Column* left = left_->evaluate(batch);
    const Column* right = right_->evaluate(batch);  // assuming literal int64_t for now

    // ASS types and sizes

    column_ = Column(left->type(), left->index());

    switch (column_.type()) {
    case Type::Int16:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int16_t>(
                static_cast<int16_t>(left->get_value<int16_t>(i) + right->get_value<int64_t>(i)));
        }
        break;
    case Type::Int32:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int32_t>(
                static_cast<int32_t>(left->get_value<int32_t>(i) + right->get_value<int64_t>(i)));
        }
        break;
    case Type::Int64:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int64_t>(
                static_cast<int64_t>(left->get_value<int64_t>(i) + right->get_value<int64_t>(i)));
        }
        break;
    case Type::Date:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int32_t>(
                static_cast<int32_t>(left->get_value<int32_t>(i) + right->get_value<int64_t>(i)));
        }
        break;
    case Type::Timestamp:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int64_t>(
                static_cast<int64_t>(left->get_value<int64_t>(i) + right->get_value<int64_t>(i)));
        }
        break;
    default:
        // wrong type
        break;
    }

    return &column_;
}

Sub::Sub(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right)
    : left_(std::move(left)), right_(std::move(right)), column_(Type::Int16, 0) {
}

const Column* Sub::evaluate(const Batch& batch) {
    const Column* left = left_->evaluate(batch);
    const Column* right = right_->evaluate(batch);  // assuming literal int64_t for now

    // ASS types and sizes

    column_ = Column(left->type(), left->index());

    switch (column_.type()) {
    case Type::Int16:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int16_t>(
                static_cast<int16_t>(left->get_value<int16_t>(i) - right->get_value<int64_t>(i)));
        }
        break;
    case Type::Int32:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int32_t>(
                static_cast<int32_t>(left->get_value<int32_t>(i) - right->get_value<int64_t>(i)));
        }
        break;
    case Type::Int64:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int64_t>(
                static_cast<int64_t>(left->get_value<int64_t>(i) - right->get_value<int64_t>(i)));
        }
        break;
    case Type::Date:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int32_t>(
                static_cast<int32_t>(left->get_value<int32_t>(i) - right->get_value<int64_t>(i)));
        }
        break;
    case Type::Timestamp:
        for (size_t i = 0; i < left->size(); ++i) {
            column_.push_value<int64_t>(
                static_cast<int64_t>(left->get_value<int64_t>(i) - right->get_value<int64_t>(i)));
        }
        break;
    default:
        // wrong type
        break;
    }

    return &column_;
}

Compare::Compare(std::unique_ptr<IValueExpression>&& left,
                 std::unique_ptr<IValueExpression>&& right, Cmp cmp)
    : left_(std::move(left)), right_(std::move(right)), cmp_(cmp) {
}

void Compare::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    const Column* left = left_->evaluate(batch);
    const Column* right = right_->evaluate(batch);

    // ASS same type for left and right

    switch (left->type()) {
    case Type::Int16:
        dispatch_int_comparator<int16_t>(*left, *right, mask);
        break;
    case Type::Int32:
        dispatch_int_comparator<int32_t>(*left, *right, mask);
        break;
    case Type::Int64:
        dispatch_int_comparator<int64_t>(*left, *right, mask);
        break;
    case Type::String:
        dispatch_string_comparator(*left, *right, mask, cmp_);
        break;
    case Type::Date:
        dispatch_int_comparator<int32_t>(*left, *right, mask);
        break;
    case Type::Timestamp:
        dispatch_int_comparator<int64_t>(*left, *right, mask);
        break;
    }
}

void Compare::dispatch_string_comparator(const Column& left, const Column& right,
                                         std::vector<uint8_t>& mask, Cmp cmp) {
    switch (cmp) {
    case Cmp::NE:
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.get_string(i) == right.get_string(i)) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::E:
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.get_string(i) != right.get_string(i)) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::GE:
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.get_string(i) < right.get_string(i)) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::LE:
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.get_string(i) > right.get_string(i)) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::G:
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.get_string(i) <= right.get_string(i)) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::L:
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.get_string(i) >= right.get_string(i)) {
                mask[i] = 0;
            }
        }
        break;
    }
}

And::And(std::unique_ptr<IFilterExpression>&& left, std::unique_ptr<IFilterExpression>&& right)
    : left_(std::move(left)), right_(std::move(right)) {
}

void And::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    left_->evaluate(batch, mask);
    right_->evaluate(batch, mask);
}

Like::Like(std::unique_ptr<IValueExpression>&& column, const std::string& pattern)
    : column_(std::move(column)), pattern_(pattern) {
}

void Like::evaluate(const Batch& batch, [[maybe_unused]] std::vector<uint8_t>& mask) {
    [[maybe_unused]] const Column* column = column_->evaluate(batch);
}

}  // namespace columnar
