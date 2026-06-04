#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>

#include "expression.hpp"

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
            const std::string& o_n = "")
        : agg_type(t), name(n), output_name(o_n.empty() ? n : o_n), expr(std::move(e)) {
    }
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

template <>
inline void dispatch_finalization<const std::string&>(Column& column, const std::string& value) {
    // ASS string type
    column.push_string(value);
}

using AggStateEntry = std::variant<CountState, CountDistinctState, StrCountDistinctState, SumState,
                                   MinState, StrMinState, MaxState, StrMaxState, AvgState>;

struct AggState {
    AggStateEntry state;

    void finalize(Column& column) const {
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
};

inline void initialize_state(AggStateEntry& state, AggType type) {
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

inline void aggregate_count(size_t row_count, CountState& state) {
    state.count += static_cast<int64_t>(row_count);
}

inline void aggregate_count_distinct(const Column& column, CountDistinctState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.set.emplace(data[i]);
            }
        },
        column);
}

inline void aggregate_count_distinct(const Column& column, CountDistinctState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.set.emplace(value);
        },
        column, row);
}

inline void aggregate_str_count_distinct(const Column& column, StrCountDistinctState& state) {
    for (size_t i = 0; i < column.size(); ++i) {
        state.str_set.emplace(column.get_string(i));
    }
}

inline void aggregate_str_count_distinct(const Column& column, StrCountDistinctState& state,
                                         size_t row) {
    state.str_set.emplace(column.get_string(row));
}

inline void aggregate_sum(const Column& column, SumState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.sum += data[i];
            }
        },
        column);
}

inline void aggregate_sum(const Column& column, SumState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.sum += value;
        },
        column, row);
}

inline void aggregate_min(const Column& column, MinState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.min = std::min(state.min, static_cast<int64_t>(data[i]));
            }
        },
        column);
}

inline void aggregate_min(const Column& column, MinState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.min = std::min(state.min, static_cast<int64_t>(value));
        },
        column, row);
}

inline void aggregate_str_min(const Column& column, StrMinState& state) {
    for (size_t i = 0; i < column.size(); ++i) {
        state.str_min = std::min(state.str_min, column.get_string(i));
    }
}

inline void aggregate_str_min(const Column& column, StrMinState& state, size_t row) {
    state.str_min = std::min(state.str_min, column.get_string(row));
}

inline void aggregate_max(const Column& column, MaxState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.max = std::max(state.max, static_cast<int64_t>(data[i]));
            }
        },
        column);
}

inline void aggregate_max(const Column& column, MaxState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.max = std::max(state.max, static_cast<int64_t>(value));
        },
        column, row);
}

inline void aggregate_str_max(const Column& column, StrMaxState& state) {
    for (size_t i = 0; i < column.size(); ++i) {
        state.str_max = std::min(state.str_max, column.get_string(i));
    }
}

inline void aggregate_str_max(const Column& column, StrMaxState& state, size_t row) {
    state.str_max = std::min(state.str_max, column.get_string(row));
}

inline void aggregate_avg(const Column& column, AvgState& state) {
    dispatch_aggregation(
        [&](const auto* data) {
            for (size_t i = 0; i < column.size(); ++i) {
                state.sum += data[i];
                state.count++;
            }
        },
        column);
}

inline void aggregate_avg(const Column& column, AvgState& state, size_t row) {
    dispatch_aggregation(
        [&](const auto& value) {
            state.sum += value;
            state.count++;
        },
        column, row);
}

// class IAggState {
// public:
//     virtual void update([[maybe_unused]] int64_t val) {
//     }

//     virtual void update([[maybe_unused]] const std::string& str) {
//     }

//     virtual void finalize(Column& column) const = 0;

//     virtual ~IAggState() = default;
// };

// class CountState : public IAggState {
// private:
//     int64_t count = 0;

// public:
//     void update([[maybe_unused]] int64_t val) override {
//         count++;
//     }

//     void finalize(Column& column) const override {
//         switch (column.type()) {
//         case Type::Int16:
//             column.push_value<int16_t>(count);
//             break;
//         case Type::Int32:
//             column.push_value<int32_t>(count);
//             break;
//         case Type::Int64:
//             column.push_value<int64_t>(count);
//             break;
//         case Type::String:
//             column.push_string(std::to_string(count));
//             break;
//         case Type::Date:
//             column.push_value<int32_t>(count);
//             break;
//         case Type::Timestamp:
//             column.push_value<int64_t>(count);
//             break;
//         }
//     }
// };

}  // namespace columnar
