#include <cstdint>
#include <cstring>
#include <memory>
#include <re2/re2.h>
#include <string>
#include <utility>
#include <variant>

#include "data_type.hpp"
#include "expression.hpp"

namespace columnar {

ColumnRef::ColumnRef(const std::string& name, std::unique_ptr<IValueExpression>&& expr)
    : name_(name), expr_(std::move(expr)) {
}

const Column* ColumnRef::evaluate(const Batch& batch) {
    if (expr_ != nullptr) {
        const Column* column = expr_->evaluate(batch);
        column_ = Column(*column);
        return &column_;
    }

    if (!batch.schema_.contains(name_) && name_ == "*") {
        column_ = Column(Type::Int64, 0);
        return &column_;
    }

    size_t idx = batch.schema_.get_column_index(name_);
    column_ = Column(batch.get_column_by_idx(idx));

    return &column_;
}

std::string ColumnRef::name() const {
    return name_;
}

Type ColumnRef::output_type() const {
    return column_.type();
}

const Column* Literal::evaluate([[maybe_unused]] const Batch& batch) {
    if (std::holds_alternative<std::string>(literal_)) {
        column_ = Column(Type::String, 0, std::get<std::string>(literal_));
        return &column_;
    }

    column_ = Column(Type::Int64, 0, std::get<int64_t>(literal_));

    return &column_;
}

Type Literal::output_type() const {
    return column_.type();
}

Add::Add(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right)
    : left_(std::move(left)), right_(std::move(right)) {
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
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
        break;
    }

    return &column_;
}

Type Add::output_type() const {
    return column_.type();
}

Sub::Sub(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right)
    : left_(std::move(left)), right_(std::move(right)) {
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
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
        break;
    }

    return &column_;
}

Type Sub::output_type() const {
    return column_.type();
}

Extract::Extract(std::unique_ptr<IValueExpression>&& target, ExtractSpec spec)
    : target_(std::move(target)), spec_(spec) {
}

const Column* Extract::evaluate(const Batch& batch) {
    const Column* column = target_->evaluate(batch);
    // ASS timestamp type
    Column extracted(Type::Int64, column->index());

    switch (spec_) {
    case ExtractSpec::second:
        for (size_t i = 0; i < column->size(); ++i) {
            extracted.push_value<int64_t>(column->get_value<int64_t>(i) % 60);
        }
        break;
    case ExtractSpec::minute:
        for (size_t i = 0; i < column->size(); ++i) {
            extracted.push_value<int64_t>((column->get_value<int64_t>(i) / 60) % 60);
        }
        break;
    case ExtractSpec::hour:
        for (size_t i = 0; i < column->size(); ++i) {
            extracted.push_value<int64_t>((column->get_value<int64_t>(i) / 3600) % 24);
        }
        break;
    }

    column_ = std::move(extracted);

    return &column_;
}

Type Extract::output_type() const {
    return column_.type();
}

StrLen::StrLen(std::unique_ptr<IValueExpression>&& target) : target_(std::move(target)) {
}

const Column* StrLen::evaluate(const Batch& batch) {
    const Column* column = target_->evaluate(batch);
    // ASS string column

    Column extracted(Type::Int32, 0);

    for (size_t i = 0; i < column->size(); ++i) {
        extracted.push_value<int32_t>(static_cast<int32_t>(column->get_string(i).size()));
    }

    column_ = std::move(extracted);

    return &column_;
}

Type StrLen::output_type() const {
    return column_.type();
}

DateTrunc::DateTrunc(std::unique_ptr<IValueExpression>&& target, TruncSpec spec)
    : target_(std::move(target)), spec_(spec) {
}

const Column* DateTrunc::evaluate(const Batch& batch) {
    const Column* column = target_->evaluate(batch);
    // ASS timestamp column

    Column truncated(Type::Timestamp, column->index());

    switch (spec_) {
    case TruncSpec::second:
        for (size_t i = 0; i < column->size(); ++i) {
            truncated.push_value<int64_t>(column->get_value<int64_t>(i));
        }
        break;
    case TruncSpec::minute:
        for (size_t i = 0; i < column->size(); ++i) {
            truncated.push_value<int64_t>((column->get_value<int64_t>(i) / 60) * 60);
        }
        break;
    case TruncSpec::hour:
        for (size_t i = 0; i < column->size(); ++i) {
            truncated.push_value<int64_t>((column->get_value<int64_t>(i) / 3600) * 3600);
        }
        break;
    }

    column_ = std::move(truncated);

    return &column_;
}

Type DateTrunc::output_type() const {
    return column_.type();
}

RegexpReplace::RegexpReplace(std::unique_ptr<IValueExpression>&& target, const std::string& pattern,
                             const std::string& replacement)
    : target_(std::move(target)), replacement_(replacement), pattern_(pattern) {
}

const Column* RegexpReplace::evaluate(const Batch& batch) {
    const Column* column = target_->evaluate(batch);
    // ASS string column

    Column replaced(Type::String, column->index());
    std::string str;

    for (size_t i = 0; i < column->size(); ++i) {
        str = column->get_string(i);
        RE2::GlobalReplace(&str, pattern_, replacement_);
        replaced.push_string(str);
    }

    column_ = std::move(replaced);

    return &column_;
}

Type RegexpReplace::output_type() const {
    return column_.type();
}

CaseWhen::CaseWhen(std::unique_ptr<IFilterExpression>&& condition,
                   std::unique_ptr<IValueExpression>&& then,
                   std::unique_ptr<IValueExpression>&& els)
    : condition_(std::move(condition)), then_(std::move(then)), else_(std::move(els)) {
}

const Column* CaseWhen::evaluate(const Batch& batch) {
    std::vector<uint8_t> mask(batch.row_count_, 1);
    condition_->evaluate(batch, mask);
    const Column* then = then_->evaluate(batch);
    const Column* els = else_->evaluate(batch);

    column_ = Column(then->type(), then->index());

    switch (column_.type()) {
    case Type::Int16:
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] == 1) {
                column_.push_value<int16_t>(then->get_value<int16_t>(i));
            } else {
                column_.push_value<int16_t>(els->get_value<int16_t>(i));
            }
        }
        break;
    case Type::Int32:
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] == 1) {
                column_.push_value<int32_t>(then->get_value<int32_t>(i));
            } else {
                column_.push_value<int32_t>(els->get_value<int32_t>(i));
            }
        }
        break;
    case Type::Int64:
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] == 1) {
                column_.push_value<int64_t>(then->get_value<int64_t>(i));
            } else {
                column_.push_value<int64_t>(els->get_value<int64_t>(i));
            }
        }
        break;
    case Type::String:
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] == 1) {
                column_.push_string(then->get_string(i));
            } else {
                column_.push_string(els->get_string(i));
            }
        }
        break;
    case Type::Date:
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] == 1) {
                column_.push_value<int32_t>(then->get_value<int32_t>(i));
            } else {
                column_.push_value<int32_t>(els->get_value<int32_t>(i));
            }
        }
        break;
    case Type::Timestamp:
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] == 1) {
                column_.push_value<int64_t>(then->get_value<int64_t>(i));
            } else {
                column_.push_value<int64_t>(els->get_value<int64_t>(i));
            }
        }
        break;
    }

    return &column_;
}

Type CaseWhen::output_type() const {
    return column_.type();
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
    size_t end = std::min(left.size(), right.size());
    std::string val = right.get_string(0);
    switch (cmp) {
    case Cmp::NE:
        for (size_t i = 0; i < end; ++i) {
            if (left.get_string(i) == val) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::E:
        for (size_t i = 0; i < end; ++i) {
            if (left.get_string(i) != val) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::GE:
        for (size_t i = 0; i < end; ++i) {
            if (left.get_string(i) < val) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::LE:
        for (size_t i = 0; i < end; ++i) {
            if (left.get_string(i) > val) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::G:
        for (size_t i = 0; i < end; ++i) {
            if (left.get_string(i) <= val) {
                mask[i] = 0;
            }
        }
        break;
    case Cmp::L:
        for (size_t i = 0; i < end; ++i) {
            if (left.get_string(i) >= val) {
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

Like::Like(std::unique_ptr<IValueExpression>&& column, const std::string& str)
    : column_(std::move(column)), pattern_(str_to_pattern(str)) {
}

void Like::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    const Column* column = column_->evaluate(batch);
    for (size_t i = 0; i < column->size(); ++i) {
        if (mask[i] == 0) {
            continue;
        }
        mask[i] &= static_cast<int>(RE2::FullMatch(column->get_string(i), pattern_));
    }
}

std::string Like::str_to_pattern(const std::string& str) {
    std::string pattern;
    for (const char& c : str) {
        switch (c) {
        case '%':
            pattern += ".*";
            break;
        case '_':
            pattern += ".";
            break;
        case '.':
        case '^':
        case '$':
        case '|':
        case '(':
        case ')':
        case '[':
        case ']':
        case '*':
        case '+':
        case '?':
        case '{':
        case '}':
        case '\\':
            pattern += "\\";
            pattern += c;
            break;
        default:
            pattern += c;
            break;
        }
    }

    return pattern;
}

NotLike::NotLike(std::unique_ptr<IValueExpression>&& column, const std::string& str)
    : column_(std::move(column)), pattern_(str_to_pattern(str)) {
}

void NotLike::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    const Column* column = column_->evaluate(batch);
    for (size_t i = 0; i < column->size(); ++i) {
        if (mask[i] == 0) {
            continue;
        }
        mask[i] &= static_cast<int>(RE2::FullMatch(column->get_string(i), pattern_));
    }
}

std::string NotLike::str_to_pattern(const std::string& str) {
    std::string pattern;
    for (const char& c : str) {
        switch (c) {
        case '%':
            pattern += ".*";
            break;
        case '_':
            pattern += ".";
            break;
        case '.':
        case '^':
        case '$':
        case '|':
        case '(':
        case ')':
        case '[':
        case ']':
        case '*':
        case '+':
        case '?':
        case '{':
        case '}':
        case '\\':
            pattern += "\\";
            pattern += c;
            break;
        default:
            pattern += c;
            break;
        }
    }

    return pattern;
}

In::In(std::unique_ptr<IValueExpression>&& column, const std::vector<int>& set)
    : column_(std::move(column)), set_(set) {
}

void In::evaluate(const Batch& batch, std::vector<uint8_t>& mask) {
    const Column* column = column_->evaluate(batch);
    mask.assign(mask.size(), 0);

    switch (column->type()) {
    case Type::Int16:
        for (size_t i = 0; i < column->size(); ++i) {
            for (size_t j = 0; j < set_.size(); ++j) {
                if (column->get_value<int16_t>(i) == set_[j]) {
                    mask[i] = 1;
                    break;
                }
            }
        }
        break;
    case Type::Int32:
        for (size_t i = 0; i < column->size(); ++i) {
            for (size_t j = 0; j < set_.size(); ++j) {
                if (column->get_value<int32_t>(i) == set_[j]) {
                    mask[i] = 1;
                    break;
                }
            }
        }
        break;
    case Type::Int64:
        for (size_t i = 0; i < column->size(); ++i) {
            for (size_t j = 0; j < set_.size(); ++j) {
                if (column->get_value<int64_t>(i) == set_[j]) {
                    mask[i] = 1;
                    break;
                }
            }
        }
        break;
    case Type::Date:
        for (size_t i = 0; i < column->size(); ++i) {
            for (size_t j = 0; j < set_.size(); ++j) {
                if (column->get_value<int32_t>(i) == set_[j]) {
                    mask[i] = 1;
                    break;
                }
            }
        }
        break;
    case Type::Timestamp:
        for (size_t i = 0; i < column->size(); ++i) {
            for (size_t j = 0; j < set_.size(); ++j) {
                if (column->get_value<int64_t>(i) == set_[j]) {
                    mask[i] = 1;
                    break;
                }
            }
        }
        break;
    default:
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
        break;
    }
}

}  // namespace columnar
