#pragma once

// #include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
// #include <expected>
#include <string>
#include <variant>
#include <vector>

namespace columnar {

using ColumnData = std::variant<std::vector<std::string>, std::vector<int64_t>>;

struct BatchColumn {
    ColumnData data;

    template <typename T>
    std::vector<T>& get() {
        return std::get<std::vector<T>>(data);
    }

    template <typename T>
    const std::vector<T>& get() const {
        return std::get<std::vector<T>>(data);
    }

    template <typename T>
    void append(const T& value) {
        get<T>().push_back(value);
    }

    size_t size() const {
        return std::visit(
            [](const auto& vec) {
                return vec.size();
            },
            data);
    }
};


class Batch {
public:
    Batch(const Schema& schema, size_t capacity = 1000);

    void add_row(const std::vector<std::string>& row);

    const BatchColumn& get_column(size_t idx) const;

    const Schema& schema() const;

    size_t get_row_count() const;

    size_t capacity() const;

    bool is_full() const;

    void clear();

private:
    const Schema& schema_;
    std::vector<BatchColumn> columns_;
    size_t capacity_;
    size_t row_count_;

    static int64_t parse_int64(const std::string& token);
};

}  // namespace columnar
