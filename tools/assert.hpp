#pragma once

#include <iostream>

#define ASS(cond, msg)                                                                             \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::cerr << "Assertion failed: (" #cond ")" << "\n  message: " << msg << "\n  at "    \
                      << __FILE__ << ":" << __LINE__ << "\n  in " << __func__ << std::endl;        \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////

// =======================================================================
// Decided not to use since most of the errors are not recoverable anyway
// =======================================================================

// namespace columnar {

// enum class Error {
//     FileNotFound         = 0,
//     InvalidSchemaFormat  = 1,
//     EmptySchema          = 2,
//     DuplicateColumn      = 3,
//     InvalidDataFormat    = 4,
//     UnsupportedType      = 5,
//     BadField             = 6,
//     FullBatch            = 7,
//     InvalidRowSize       = 8,
//     IndexOutOfRange      = 9,
//     BadVariantAccess     = 10,
//     NotImplemented       = 11,
//     DEBUG                = 12,
// };

// inline const char* to_string(Error type) {
//     switch (type) {
//     case Error::FileNotFound:
//         return "file_not_found";
//     case Error::InvalidSchemaFormat:
//         return "invalid_schema_format";
//     case Error::UnsupportedType:
//         return "unsupported_type";
//     case Error::EmptySchema:
//         return "empty_schema";
//     case Error::DuplicateColumn:
//         return "duplicate_column";
//     case Error::FullBatch:
//         return "full_batch";
//     case Error::InvalidRowSize:
//         return "invalid_row_size";
//     case Error::IndexOutOfRange:
//         return "index_out_of_range";
//     case Error::InvalidDataFormat:
//         return "invalid_data_format";
//     case Error::NotImplemented:
//         return "not_implemented";
//     case Error::BadField:
//         return "bad_field";
//     case Error::BadVariantAccess:
//         return "bad_variant_access";
//     case Error::DEBUG:
//         return "DEBUUUUUUUG";
//     default:
//         return "UNKNOWN_ERROR";
//     }
// }

// inline std::ostream& operator<<(std::ostream& os, Error type) {
//     return os << to_string(type);
// }

// template <typename T>
// class Expected {
// private:
//     std::expected<T, Error> value_{};

// public:
//     Expected(std::unexpected<Error> e) : value_(e) {
//     }

//     Expected(T& val) : value_(val) {
//     }

//     Expected(T&& val) : value_(std::move(val)) {
//     }

//     T& operator*() & {
//         return *value_;
//     }

//     const T& operator*() const& {
//         return *value_;
//     }

//     T&& operator*() && {
//         return std::move(*value_);
//     }

//     const T&& operator*() const&& {
//         return std::move(*value_);
//     }

//     T* operator->() {
//         return &(*value_);
//     }

//     const T* operator->() const {
//         return &(*value_);
//     }

//     Error& error() {
//         return value_.error();
//     }

//     const Error& error() const {
//         return value_.error();
//     }

//     bool has_value() const {
//         return value_.has_value();
//     }
// };

// template <typename T>
// class Expected<T&> {
// private:
//     std::expected<std::reference_wrapper<T>, Error> value_{};

// public:
//     Expected(std::unexpected<Error> e) : value_(e) {
//     }

//     Expected(T& val) : value_(val) {
//     }

//     T& operator*() & {
//         return value_->get();
//     }

//     const T& operator*() const& {
//         return value_->get();
//     }

//     T&& operator*() && {
//         return std::move(value_->get());
//     }

//     const T&& operator*() const&& {
//         return std::move(value_->get());
//     }

//     T* operator->() {
//         return &(value_->get());
//     }

//     const T* operator->() const {
//         return &(value_->get());
//     }

//     Error& error() {
//         return value_.error();
//     }

//     const Error& error() const {
//         return value_.error();
//     }

//     bool has_value() const {
//         return value_.has_value();
//     }
// };

// template <>
// class Expected<void> {
// private:
//     std::expected<void, Error> value_{};

// public:
//     Expected() : value_() {
//     }

//     Expected(std::unexpected<Error> e) : value_(e) {
//     }

//     Error& error() {
//         return value_.error();
//     }

//     const Error& error() const {
//         return value_.error();
//     }

//     bool has_value() const {
//         return value_.has_value();
//     }
// };

// }  // namespace columnar
