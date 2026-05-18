#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "assert.hpp"
#include "config.hpp"
#include "schema.hpp"

namespace columnar {

using ColumnData = std::variant<std::vector<int16_t>, std::vector<int32_t>, std::vector<int64_t>,
                                std::vector<std::string>, std::vector<char>>;

struct BatchColumn {
    ColumnData data;

    template <typename T>
    std::vector<T>& get() {
        return std::get<std::vector<T>>(data);  // throws, get_if?
    }

    template <typename T>
    const std::vector<T>& get() const {
        return std::get<std::vector<T>>(data);  // throws, get_if?
    }

    template <typename T>
    void append(const T& value) {
        std::get<std::vector<T>>(data).push_back(value);  // throws, get_if?
    }
};

class ConversionBatch {
private:
    const Schema& schema_;
    std::vector<BatchColumn> columns_;
    size_t capacity_ = kConversionBatchCapacity;
    size_t row_count_ = 0;

public:
    ConversionBatch(const Schema& schema, size_t capacity = kConversionBatchCapacity);  // throws

    void add_row(const std::vector<std::string>& row);  // throws

    const BatchColumn& get_column(size_t idx) const;  // throws

    template <typename T>
    void append_column(size_t idx, T value) {
        ASS(idx < columns_.size(), "index out of range");
        columns_[idx].append<T>(value);  // throws
    }

    void set_row_count(size_t num);

    size_t get_row_count() const;

    bool is_full() const;

    void clear();  // throws

private:
    template <typename T, typename ParseFn>
    static void parse_and_append(BatchColumn& column, const std::string& token,
                                 ParseFn parse_fn);  // throws
};

}  // namespace columnar
