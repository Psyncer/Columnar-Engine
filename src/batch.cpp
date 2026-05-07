#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "assert.hpp"
#include "batch.hpp"
#include "data_type.hpp"
#include "parsing.hpp"
#include "schema.hpp"

namespace columnar {

Batch::Batch(const Schema& schema, size_t capacity) : schema_(schema), capacity_(capacity) {
    for (size_t i = 0; i < schema_.get_column_count(); ++i) {
        SchemaColumn column = schema_.get_column(i);

        switch (column.type_) {
        case Type::Int16: {
            std::vector<int16_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        case Type::Int32: {
            std::vector<int32_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        case Type::Int64: {
            std::vector<int64_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        case Type::String: {
            std::vector<std::string> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        case Type::Date: {
            std::vector<int32_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        case Type::Timestamp: {
            std::vector<int64_t> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        case Type::Char: {
            std::vector<char> vec;
            vec.reserve(capacity_);
            columns_.emplace_back(std::move(vec));
            break;
        }
        default:
            std::cerr << "Type not implemented" << "\n  at " << __FILE__ << ":" << __LINE__
                      << "\n  in " << __func__ << std::endl;
            std::abort();
        }
    }
}

void Batch::add_row(const std::vector<std::string>& row) {
    for (size_t i = 0; i < row.size(); ++i) {
        Type type = schema_.get_column(i).type_;

        switch (type) {
        case Type::Int16: {
            parse_and_append<int16_t>(columns_[i], row[i], parse_int<int16_t>);
            break;
        }
        case Type::Int32: {
            parse_and_append<int32_t>(columns_[i], row[i], parse_int<int32_t>);
            break;
        }
        case Type::Int64: {
            parse_and_append<int64_t>(columns_[i], row[i], parse_int<int64_t>);
            break;
        }
        case Type::String: {
            columns_[i].append<std::string>(row[i]);
            break;
        }
        case Type::Date: {
            parse_and_append<int32_t>(columns_[i], row[i], parse_date);
            break;
        }
        case Type::Timestamp: {
            parse_and_append<int64_t>(columns_[i], row[i], parse_timestamp);
            break;
        }
        case Type::Char: {
            parse_and_append<char>(columns_[i], row[i], parse_char);
            break;
        }
        default:
            std::cerr << "Type not implemented" << "\n  at " << __FILE__ << ":" << __LINE__
                      << "\n  in " << __func__ << std::endl;
            std::abort();
        }
    }

    row_count_++;
}

const BatchColumn& Batch::get_column(size_t idx) const {
    ASS(idx < columns_.size(), "index out of range");
    return columns_[idx];
}

void Batch::set_row_count(size_t num) {
    row_count_ = num;
}

size_t Batch::get_row_count() const {
    // maybe assert row_count_ invariant here
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

template <typename T, typename ParseFn>
void Batch::parse_and_append(BatchColumn& column, const std::string& token, ParseFn parse_fn) {
    auto value = parse_fn(token);
    column.append<T>(value);
}

}  // namespace columnar
