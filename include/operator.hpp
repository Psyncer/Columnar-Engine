#pragma once

#include <memory>

#include "aggregate.hpp"
#include "batch.hpp"
#include "columnar_reader.hpp"
#include "expression.hpp"

namespace columnar {

class IOperator {
public:
    virtual Batch* next() = 0;
    virtual ~IOperator() = default;
    virtual const Schema& get_schema() const = 0;
};

class ScanOperator : public IOperator {
private:
    ColumnarReader reader_;
    Schema schema_;
    Batch batch_;

public:
    ScanOperator(const std::string& path, const std::vector<std::string>& columns_needed);

    Batch* next() override;

    const Schema& get_schema() const override;
};

class GlobalAggOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    std::vector<AggSpec> specs_;
    std::vector<AggState> states_;
    Batch result_batch_;
    Schema result_schema_;
    bool done_ = false;

public:
    GlobalAggOperator(std::unique_ptr<IOperator>&& child, std::vector<AggSpec> specs);

    Batch* next() override;

    const Schema& get_schema() const override;
};

class FilterOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    std::unique_ptr<IFilterExpression> filter_;
    Batch result_batch_;

public:
    FilterOperator(std::unique_ptr<IOperator>&& child, std::unique_ptr<IFilterExpression>&& filter);

    Batch* next() override;

    const Schema& get_schema() const override;

private:
    static void apply_mask(Batch* batch, std::vector<uint8_t>& mask);
};

class GroupByOperator : public IOperator {
    struct GroupKey {
        std::vector<int64_t> ints;
        std::vector<std::string> strs;

        bool operator==(const GroupKey& other) const {
            return ints == other.ints && strs == other.strs;
        }
    };

    struct KeyHash {
        size_t operator()(const GroupKey& key) const {
            size_t seed = 0;

            for (int64_t v : key.ints) {
                hash_combine(seed, std::hash<int64_t>{}(v));
            }

            for (const auto& s : key.strs) {
                hash_combine(seed, std::hash<std::string>{}(s));
            }

            return seed;
        }

        static void hash_combine(size_t& seed, size_t v) {
            seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        }
    };

private:
    std::unique_ptr<IOperator> child_;
    std::vector<std::unique_ptr<IValueExpression>> group_exprs_;
    std::vector<AggSpec> specs_;
    std::unordered_map<GroupKey, std::vector<AggState>, KeyHash> groups_;
    Batch result_batch_;
    Schema result_schema_;
    bool done_ = false;

public:
    GroupByOperator(std::unique_ptr<IOperator>&& child,
                    std::vector<std::unique_ptr<IValueExpression>> group_exprs,
                    std::vector<AggSpec> specs);

    Batch* next() override;

    const Schema& get_schema() const override;
};

class OrderByOperator : public IOperator {
public:
    enum class OrderDirection {
        Desc,
        Asc,
    };

    struct OrderSpec {
        std::unique_ptr<IValueExpression> expr;
        OrderDirection dir;
    };

private:
    std::unique_ptr<IOperator> child_;
    std::vector<OrderSpec> order_specs_;
    Batch accumulated_batch_;
    Batch result_batch_;
    bool done_ = false;

public:
    OrderByOperator(std::unique_ptr<IOperator> child, std::vector<OrderSpec> order_specs);

    Batch* next() override;

    const Schema& get_schema() const override;
};

class LimitOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    size_t len_;
    size_t offset_;

public:
    LimitOperator(std::unique_ptr<IOperator> child, size_t len, size_t offset = 0);

    Batch* next() override;

    const Schema& get_schema() const override;
};

}  // namespace columnar
