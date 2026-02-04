#pragma once

// #include "parse_error.hpp"
#include "schema.hpp"

#include <cstdint>
// #include <expected>
#include <string>
#include <variant>
#include <vector>

namespace columnar {

/*
Такая реализация хранилища колонок при конвертации временная, буду менять.
*/
using ColumnData = std::variant<std::vector<std::string>, std::vector<int64_t>>;

class Batch {
public:
    Batch(const Schema& schema, size_t capacity = 1000);

    void add_row(const std::vector<std::string>& row);

    void add_column(const ColumnData& column);

    const ColumnData& get_column(size_t idx) const;

    const Schema& schema() const;

    size_t get_row_count() const;

    size_t capacity() const;

    bool is_full() const;

    void clear();

private:
    const Schema& schema_;
    std::vector<ColumnData> columns_;
    size_t capacity_;
    size_t row_count_;

    static int64_t parse_int64(const std::string& token);
};

}  // namespace columnar
