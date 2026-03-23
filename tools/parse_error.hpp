#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <ostream>

namespace columnar {

enum class parse_error : int64_t {
    file_not_found = 0,
    invalid_schema_format = 1,
    unsupported_type = 2,
    empty_schema = 3,
    duplicate_column = 4,
    full_batch = 5,
    invalid_row_size = 6,
    index_out_of_range = 7,
    invalid_data_format = 8,
    not_implemented = 9,
    bad_field = 10,
    bad_variant_access = 11,
};

inline const char* to_string(parse_error type) {
    switch (type) {
    case parse_error::file_not_found:
        return "file_not_found";
    case parse_error::invalid_schema_format:
        return "invalid_schema_format";
    case parse_error::unsupported_type:
        return "unsupported_type";
    case parse_error::empty_schema:
        return "empty_schema";
    case parse_error::duplicate_column:
        return "duplicate_column";
    case parse_error::full_batch:
        return "full_batch";
    case parse_error::invalid_row_size:
        return "invalid_row_size";
    case parse_error::index_out_of_range:
        return "index_out_of_range";
    case parse_error::invalid_data_format:
        return "invalid_data_format";
    case parse_error::not_implemented:
        return "not_implemented";
    case parse_error::bad_field:
        return "bad_field";
    case parse_error::bad_variant_access:
        return "bad_variant_access";
    default:
        return "UNKNOWN_ERROR";
    }
}

inline std::ostream& operator<<(std::ostream& os, parse_error type) {
    return os << to_string(type);
}

template <typename T>
class Expected {
private:
    std::expected<T, parse_error> value_{};

public:
    Expected(std::unexpected<parse_error> e) : value_(e) {
    }

    Expected(T& val) : value_(val) {
    }

    Expected(T&& val) : value_(std::move(val)) {
    }

    T& operator*() & {
        return *value_;
    }

    const T& operator*() const& {
        return *value_;
    }

    T&& operator*() && {
        return std::move(*value_);
    }

    const T&& operator*() const&& {
        return std::move(*value_);
    }

    T* operator->() {
        return &(*value_);
    }

    const T* operator->() const {
        return &(*value_);
    }

    parse_error& error() {
        return value_.error();
    }

    const parse_error& error() const {
        return value_.error();
    }

    bool has_value() const {
        return value_.has_value();
    }
};

template <typename T>
class Expected<T&> {
private:
    std::expected<std::reference_wrapper<T>, parse_error> value_{};

public:
    Expected(std::unexpected<parse_error> e) : value_(e) {
    }

    Expected(T& val) : value_(val) {
    }

    T& operator*() & {
        return value_->get();
    }

    const T& operator*() const& {
        return value_->get();
    }

    T&& operator*() && {
        return std::move(value_->get());
    }

    const T&& operator*() const&& {
        return std::move(value_->get());
    }

    T* operator->() {
        return &(value_->get());
    }

    const T* operator->() const {
        return &(value_->get());
    }

    parse_error& error() {
        return value_.error();
    }

    const parse_error& error() const {
        return value_.error();
    }

    bool has_value() const {
        return value_.has_value();
    }
};

template <>
class Expected<void> {
private:
    std::expected<void, parse_error> value_{};

public:
    Expected() : value_() {
    }

    Expected(std::unexpected<parse_error> e) : value_(e) {
    }

    parse_error& error() {
        return value_.error();
    }

    const parse_error& error() const {
        return value_.error();
    }

    bool has_value() const {
        return value_.has_value();
    }
};

}  // namespace columnar
