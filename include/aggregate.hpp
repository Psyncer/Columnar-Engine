#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>

#include "column.hpp"

namespace columnar {

enum class AggType {
    Count,
    CountDistinct,
    StrCountDistinct,
    Sum,
    Min,
    StrMin,
    Max,
    StrMax,
    Avg,
};

struct AggSpec {
    AggType type;
    std::string name;
};

struct AggState {
    int64_t count = 0;
    __int128 sum = 0;
    int64_t min = std::numeric_limits<int64_t>::max();
    int64_t max = std::numeric_limits<int64_t>::min();
    std::unordered_set<int64_t> distinct;

    std::string str_min = std::string(16, char(0xFF));
    std::string str_max;
    std::unordered_set<std::string> str_distinct;

    void finalize(Column& column, AggType type) const {
        switch (type) {
        case AggType::Count:
            column.push_value(count);
            break;
        case AggType::CountDistinct:
            column.push_value(distinct.size());
            break;
        case AggType::StrCountDistinct:
            column.push_value(str_distinct.size());
            break;
        case AggType::Sum:
            column.push_value(static_cast<int64_t>(sum));
            break;
        case AggType::Min:
            column.push_value(min);
            break;
        case AggType::StrMin:
            column.push_string(str_min);
            break;
        case AggType::Max:
            column.push_value(max);
            break;
        case AggType::StrMax:
            column.push_string(str_max);
            break;
        case AggType::Avg:
            column.push_value(static_cast<int64_t>(sum / count));
            break;
        }
    }
};

template <template <typename> class AggFn>
void dispatch_agg(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
    switch (column.type()) {
    case Type::Int16:
        AggFn<int16_t>{}(column, state, active_rows);
        break;
    case Type::Int32:
        AggFn<int32_t>{}(column, state, active_rows);
        break;
    case Type::Int64:
        AggFn<int64_t>{}(column, state, active_rows);
        break;
    case Type::String:
        AggFn<char>{}(column, state, active_rows);
        break;
    case Type::Date:
        AggFn<int32_t>{}(column, state, active_rows);
        break;
    case Type::Timestamp:
        AggFn<int64_t>{}(column, state, active_rows);
        break;
    }
}

class AggCount {
public:
    void operator()(size_t row_count, AggState& state) {
        state.count += static_cast<int64_t>(row_count);
    }
};

template <typename T>
class AggCountDistinct {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        const T* data = column.data_as<T>();
        for (const auto& row : active_rows) {
            state.distinct.insert(data[row]);
        }
    }
};

template <>
class AggCountDistinct<char> {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        for (const auto& row : active_rows) {
            state.str_distinct.insert(column.get_string(row));
        }
    }
};

template <typename T>
class AggSum {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        const T* data = column.data_as<T>();
        for (const auto& row : active_rows) {
            state.sum += data[row];
            // data[row] = 5;  // DELETE
        }
    }
};

template <typename T>
class AggMin {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        const T* data = column.data_as<T>();
        int64_t cur_min = state.min;
        for (const auto& row : active_rows) {
            if (data[row] < cur_min) {
                cur_min = data[row];
            }
        }
        state.min = cur_min;
    }
};

template <>
class AggMin<char> {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        std::string cur_min = state.str_min;
        for (const auto& row : active_rows) {
            if (column.get_string(row) < cur_min) {
                cur_min = column.get_string(row);
            }
        }
        state.str_min = cur_min;
    }
};

template <typename T>
class AggMax {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        const T* data = column.data_as<T>();
        int64_t cur_max = state.max;
        for (const auto& row : active_rows) {
            if (data[row] > cur_max) {
                cur_max = data[row];
            }
        }
        state.max = cur_max;
    }
};

template <>
class AggMax<char> {
public:
    void operator()(const Column& column, AggState& state, const std::vector<size_t>& active_rows) {
        std::string cur_max = state.str_max;
        for (const auto& row : active_rows) {
            if (column.get_string(row) > cur_max) {
                cur_max = column.get_string(row);
            }
        }
        state.str_min = cur_max;
    }
};

}  // namespace columnar
