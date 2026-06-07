#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <variant>

#include "src/execution/expression.hpp"

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
    AggType agg_type;
    std::string name;
    std::string output_name;
    std::unique_ptr<IValueExpression> expr;

    AggSpec(AggType t, const std::string& n, std::unique_ptr<IValueExpression> e,
            const std::string& o_n = "");
};

struct CountState {
    int64_t count = 0;
};

struct CountDistinctState {
    std::unordered_set<int64_t> set;
};

struct StrCountDistinctState {
    std::unordered_set<std::string> str_set;
};

struct SumState {
    __int128_t sum = 0;
};

struct MinState {
    int64_t min = std::numeric_limits<int64_t>::max();
};

struct StrMinState {
    std::string str_min = std::string(16, char(0xFF));
};

struct MaxState {
    int64_t max = std::numeric_limits<int64_t>::min();
};

struct StrMaxState {
    std::string str_max;
};

struct AvgState {
    __int128_t sum = 0;
    int64_t count = 0;
};

using AggStateEntry = std::variant<CountState, CountDistinctState, StrCountDistinctState, SumState,
                                   MinState, StrMinState, MaxState, StrMaxState, AvgState>;

template <typename T>
void dispatch_finalization(Column& column, const T& value) {
    switch (column.type()) {
    case Type::Int16:
        column.push_value<int16_t>(value);
        break;
    case Type::Int32:
        column.push_value<int32_t>(value);
        break;
    case Type::Int64:
        column.push_value<int64_t>(value);
        break;
    case Type::Date:
        column.push_value<int32_t>(value);
        break;
    case Type::Timestamp:
        column.push_value<int64_t>(value);
        break;
    default:
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
    }
}

struct AggState {
    AggStateEntry state;

    void finalize(Column& column) const;
};

void initialize_state(AggStateEntry& state, AggType type);

template <typename Func>
void dispatch_aggregation(Func&& f, const Column& column) {
    switch (column.type()) {
    case Type::Int16:
        f(column.data_as<int16_t>());
        break;
    case Type::Int32:
        f(column.data_as<int32_t>());
        break;
    case Type::Int64:
        f(column.data_as<int64_t>());
        break;
    case Type::Date:
        f(column.data_as<int32_t>());
        break;
    case Type::Timestamp:
        f(column.data_as<int64_t>());
        break;
    default:
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
    }
}

template <typename Func>
void dispatch_aggregation(Func&& f, const Column& column, size_t row) {
    switch (column.type()) {
    case Type::Int16:
        f(column.get_value<int16_t>(row));
        break;
    case Type::Int32:
        f(column.get_value<int32_t>(row));
        break;
    case Type::Int64:
        f(column.get_value<int64_t>(row));
        break;
    case Type::Date:
        f(column.get_value<int32_t>(row));
        break;
    case Type::Timestamp:
        f(column.get_value<int64_t>(row));
        break;
    default:
        std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                  << __func__ << std::endl;
        std::abort();
    }
}

void aggregate_count(size_t row_count, CountState& state);

void aggregate_count_distinct(const Column& column, CountDistinctState& state);

void aggregate_count_distinct(const Column& column, CountDistinctState& state, size_t row);

void aggregate_str_count_distinct(const Column& column, StrCountDistinctState& state);

void aggregate_str_count_distinct(const Column& column, StrCountDistinctState& state, size_t row);

void aggregate_sum(const Column& column, SumState& state);

void aggregate_sum(const Column& column, SumState& state, size_t row);

void aggregate_min(const Column& column, MinState& state);

void aggregate_min(const Column& column, MinState& state, size_t row);

void aggregate_str_min(const Column& column, StrMinState& state);

void aggregate_str_min(const Column& column, StrMinState& state, size_t row);

void aggregate_max(const Column& column, MaxState& state);

void aggregate_max(const Column& column, MaxState& state, size_t row);

void aggregate_str_max(const Column& column, StrMaxState& state);

void aggregate_str_max(const Column& column, StrMaxState& state, size_t row);

void aggregate_avg(const Column& column, AvgState& state);

void aggregate_avg(const Column& column, AvgState& state, size_t row);

}  // namespace columnar
