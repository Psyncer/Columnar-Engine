#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>

#include "src/execution/aggregate.hpp"
#include "src/execution/expression.hpp"

namespace columnar {

AggSpec::AggSpec(AggType t, const std::string& n, std::unique_ptr<IValueExpression> e,
                 const std::string& o_n)
    : agg_type(t), name(n), output_name(o_n.empty() ? n : o_n), expr(std::move(e)) {
}

template <>
void dispatch_finalization<const std::string&>(Column& column, const std::string& value) {
    // ASS string type
    column.push_string(value);
}

void AggState::finalize(Column& column) const {
    std::visit(
        [&](const auto& st) {
            using T = std::decay_t<decltype(st)>;
            if constexpr (std::is_same_v<T, CountState>) {
                dispatch_finalization(column, st.count);
            } else if constexpr (std::is_same_v<T, CountDistinctState>) {
                dispatch_finalization(column, st.set.size());
            } else if constexpr (std::is_same_v<T, StrCountDistinctState>) {
                dispatch_finalization<const std::string&>(column,
                                                          std::to_string(st.str_set.size()));
            } else if constexpr (std::is_same_v<T, SumState>) {
                dispatch_finalization(column, st.sum);
            } else if constexpr (std::is_same_v<T, MinState>) {
                dispatch_finalization(column, st.min);
            } else if constexpr (std::is_same_v<T, StrMinState>) {
                dispatch_finalization<const std::string&>(column, st.str_min);
            } else if constexpr (std::is_same_v<T, MaxState>) {
                dispatch_finalization(column, st.max);
            } else if constexpr (std::is_same_v<T, StrMaxState>) {
                dispatch_finalization<const std::string&>(column, st.str_max);
            } else if constexpr (std::is_same_v<T, AvgState>) {
                dispatch_finalization(column, (st.sum / st.count));
            }
        },
        state);
}

void initialize_state(AggStateEntry& state, AggType type) {
    switch (type) {
    case AggType::Count:
        state.emplace<CountState>();
        break;
    case AggType::CountDistinct:
        state.emplace<CountDistinctState>();
        break;
    case AggType::StrCountDistinct:
        state.emplace<StrCountDistinctState>();
        break;
    case AggType::Sum:
        state.emplace<SumState>();
        break;
    case AggType::Min:
        state.emplace<MinState>();
        break;
    case AggType::StrMin:
        state.emplace<StrMinState>();
        break;
    case AggType::Max:
        state.emplace<MaxState>();
        break;
    case AggType::StrMax:
        state.emplace<StrMaxState>();
        break;
    case AggType::Avg:
        state.emplace<AvgState>();
        break;
    }
}

void aggregate_count(size_t row_count, CountState& state) {
    state.count += static_cast<int64_t>(row_count);
}

void aggregate_count_distinct(const Column& column, CountDistinctState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.set.emplace(data[i]);
            }
        },
        column);
}

void aggregate_count_distinct(const Column& column, CountDistinctState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.set.emplace(value);
        },
        column, row);
}

void aggregate_str_count_distinct(const Column& column, StrCountDistinctState& state) {
    for (size_t i = 0; i < column.size(); ++i) {
        state.str_set.emplace(column.get_string(i));
    }
}

void aggregate_str_count_distinct(const Column& column, StrCountDistinctState& state, size_t row) {
    state.str_set.emplace(column.get_string(row));
}

void aggregate_sum(const Column& column, SumState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.sum += data[i];
            }
        },
        column);
}

void aggregate_sum(const Column& column, SumState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.sum += value;
        },
        column, row);
}

void aggregate_min(const Column& column, MinState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.min = std::min(state.min, static_cast<int64_t>(data[i]));
            }
        },
        column);
}

void aggregate_min(const Column& column, MinState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.min = std::min(state.min, static_cast<int64_t>(value));
        },
        column, row);
}

void aggregate_str_min(const Column& column, StrMinState& state) {
    for (size_t i = 0; i < column.size(); ++i) {
        state.str_min = std::min(state.str_min, column.get_string(i));
    }
}

void aggregate_str_min(const Column& column, StrMinState& state, size_t row) {
    state.str_min = std::min(state.str_min, column.get_string(row));
}

void aggregate_max(const Column& column, MaxState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.max = std::max(state.max, static_cast<int64_t>(data[i]));
            }
        },
        column);
}

void aggregate_max(const Column& column, MaxState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.max = std::max(state.max, static_cast<int64_t>(value));
        },
        column, row);
}

void aggregate_str_max(const Column& column, StrMaxState& state) {
    for (size_t i = 0; i < column.size(); ++i) {
        state.str_max = std::min(state.str_max, column.get_string(i));
    }
}

void aggregate_str_max(const Column& column, StrMaxState& state, size_t row) {
    state.str_max = std::min(state.str_max, column.get_string(row));
}

void aggregate_avg(const Column& column, AvgState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.sum += data[i];
                state.count++;
            }
        },
        column);
}

void aggregate_avg(const Column& column, AvgState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.sum += value;
            state.count++;
        },
        column, row);
}

}  // namespace columnar
