#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>

#include "column.hpp"
#include "data_type.hpp"
#include "expression.hpp"

namespace columnar {

class IAggState;

using AggStatePtr = std::unique_ptr<IAggState>;

enum class AggType {
    Count,
    CountDistinct,
    Sum,
    Min,
    Max,
    Avg,
};

namespace delete_later {

inline const char* agg_to_str(AggType type) {
    switch (type) {
    case AggType::Count:
        return "COUNT";
    case AggType::CountDistinct:
        return "COUNT DISTINCT";
    case AggType::Sum:
        return "SUM";
    case AggType::Min:
        return "MIN";
    case AggType::Max:
        return "MAX";
    case AggType::Avg:
        return "AVG";
    }
}

inline std::ostream& operator<<(std::ostream& os, Type type) {
    return os << to_string(type);
}

}  // namespace delete_later

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

class IAggState {
public:
    virtual void update([[maybe_unused]] int64_t val) {
    }

    virtual void update([[maybe_unused]] const std::string& str) {
    }

    virtual void finalize(Column& column) const = 0;

    virtual ~IAggState() = default;
};

class CountState : public IAggState {
private:
    int64_t count = 0;

public:
    void update([[maybe_unused]] int64_t val) override {
        count++;
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(count);
            break;
        case Type::Int32:
            column.push_value<int32_t>(count);
            break;
        case Type::Int64:
            column.push_value<int64_t>(count);
            break;
        case Type::String:
            column.push_string(std::to_string(count));
            break;
        case Type::Date:
            column.push_value<int32_t>(count);
            break;
        case Type::Timestamp:
            column.push_value<int64_t>(count);
            break;
        }
    }
};

class CountDistinctState : public IAggState {
private:
    std::unordered_set<int64_t> distinct;

public:
    void update(int64_t val) override {
        distinct.insert(val);
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(distinct.size());
            break;
        case Type::Int32:
            column.push_value<int32_t>(distinct.size());
            break;
        case Type::Int64:
            column.push_value<int64_t>(distinct.size());
            break;
        case Type::Date:
            column.push_value<int32_t>(distinct.size());
            break;
        case Type::Timestamp:
            column.push_value<int64_t>(distinct.size());
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class StrCountDistinctState : public IAggState {
private:
    std::unordered_set<std::string> distinct;

public:
    void update(const std::string& str) override {
        distinct.insert(str);
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(distinct.size());
            break;
        case Type::Int32:
            column.push_value<int32_t>(distinct.size());
            break;
        case Type::Int64:
            column.push_value<int64_t>(distinct.size());
            break;
        case Type::Date:
            column.push_value<int32_t>(distinct.size());
            break;
        case Type::Timestamp:
            column.push_value<int64_t>(distinct.size());
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class SumState : public IAggState {
private:
    __int128_t sum = 0;

public:
    void update(int64_t val) override {
        sum += val;
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(sum);
            break;
        case Type::Int32:
            column.push_value<int32_t>(sum);
            break;
        case Type::Int64:
            column.push_value<int64_t>(sum);
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class MinState : public IAggState {
private:
    int64_t min = std::numeric_limits<int64_t>::max();

public:
    void update(int64_t val) override {
        if (val < min) {
            min = val;
        }
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(min);
            break;
        case Type::Int32:
            column.push_value<int32_t>(min);
            break;
        case Type::Int64:
            column.push_value<int64_t>(min);
            break;
        case Type::Date:
            column.push_value<int32_t>(min);
            break;
        case Type::Timestamp:
            column.push_value<int64_t>(min);
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class StrMinState : public IAggState {
private:
    std::string str_min = std::string(16, char(0xFF));

public:
    void update(const std::string& str) override {
        if (str < str_min) {
            str_min = str;
        }
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::String:
            column.push_string(str_min);
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class MaxState : public IAggState {
private:
    int64_t max = std::numeric_limits<int64_t>::min();

public:
    void update(int64_t val) override {
        if (val > max) {
            max = val;
        }
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(max);
            break;
        case Type::Int32:
            column.push_value<int32_t>(max);
            break;
        case Type::Int64:
            column.push_value<int64_t>(max);
            break;
        case Type::Date:
            column.push_value<int32_t>(max);
            break;
        case Type::Timestamp:
            column.push_value<int64_t>(max);
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class StrMaxState : public IAggState {
private:
    std::string str_max;

public:
    void update(const std::string& str) override {
        if (str > str_max) {
            str_max = str;
        }
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::String:
            column.push_string(str_max);
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

class AvgState : public IAggState {
private:
    __int128_t sum = 0;
    int64_t count = 0;

public:
    void update(int64_t val) override {
        sum += val;
        count++;
    }

    void finalize(Column& column) const override {
        switch (column.type()) {
        case Type::Int16:
            column.push_value<int16_t>(sum / count);
            break;
        case Type::Int32:
            column.push_value<int32_t>(sum / count);
            break;
        case Type::Int64:
            column.push_value<int64_t>(sum / count);
            break;
        case Type::Date:
            column.push_value<int32_t>(sum / count);
            break;
        case Type::Timestamp:
            column.push_value<int64_t>(sum / count);
            break;
        default:
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }
};

inline AggStatePtr make_state(AggType agg_type, Type col_type) {
    switch (agg_type) {
    case AggType::Count:
        return std::make_unique<CountState>();
    case AggType::Sum:
        return std::make_unique<SumState>();
    case AggType::Min:
        if (col_type == Type::String) {
            return std::make_unique<StrMinState>();
        } else {
            return std::make_unique<MinState>();
        }
    case AggType::Max:
        if (col_type == Type::String) {
            return std::make_unique<StrMaxState>();
        } else {
            return std::make_unique<MaxState>();
        }
    case AggType::Avg:
        return std::make_unique<AvgState>();

    case AggType::CountDistinct:
        if (col_type == Type::String) {
            return std::make_unique<StrCountDistinctState>();
        } else {
            return std::make_unique<CountDistinctState>();
        }

        return nullptr;
    }
}

}  // namespace columnar
