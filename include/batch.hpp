#pragma once

#include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <variant>
#include <vector>

namespace columnar {

// add remaining types and then try the buffer
using ColumnData = std::variant<std::vector<std::string>, std::vector<int64_t>>;

struct BatchColumn {
    ColumnData data{};

    template <typename T>
    Expected<std::vector<T>&> get() {
        try {
            return std::get<std::vector<T>>(data);
        } catch (const std::exception& e) {
            return std::unexpected(parse_error::bad_variant_access);
        }
    }

    template <typename T>
    Expected<const std::vector<T>&> get() const {
        try {
            return std::get<std::vector<T>>(data);
        } catch (const std::exception& e) {
            return std::unexpected(parse_error::bad_variant_access);
        }
    }

    template <typename T>
    Expected<void> append(const T& value) {
        try {
            std::get<std::vector<T>>(data).push_back(value);
        } catch (const std::exception& e) {
            return std::unexpected(parse_error::bad_variant_access);
        }

        return {};
    }
};

class Batch {
private:
    const Schema& schema_{};
    std::vector<BatchColumn> columns_{};
    size_t capacity_{};
    size_t row_count_{};

public:
    Batch(const Schema& schema, size_t capacity = 1000);

    Expected<void> add_row(const std::vector<std::string>& row);

    Expected<const BatchColumn&> get_column(size_t idx) const;

    size_t get_row_count() const;

    bool is_full() const;

    void clear();
};

}  // namespace columnar
