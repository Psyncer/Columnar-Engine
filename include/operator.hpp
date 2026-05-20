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
    using GroupKey = std::vector<int64_t>;
    using GroupAggState = std::vector<AggState>;

private:
    std::unique_ptr<IOperator> child_;
    std::vector<std::shared_ptr<IValueExpression>> exprs_;
    std::vector<AggSpec> agg_specs_;
    Batch result_batch_;
    std::unordered_map<GroupKey, GroupAggState> hash_table_;
    bool done_ = false;

public:
    GroupByOperator(std::unique_ptr<IOperator>&& child,
                    std::vector<std::shared_ptr<IValueExpression>> exprs,
                    std::vector<AggSpec> agg_specs);

    Batch* next() override;

    const Schema& get_schema() const override;
};

class OrderByOperator : public IOperator {
    enum class OrderDirection {
        Desc,
        Asc,
    };

    struct OrderSpec {
        OrderDirection dir;
        std::shared_ptr<IValueExpression> expr;
    };

private:
    std::unique_ptr<IOperator> child_;
    std::vector<OrderSpec> order_specs_;

public:
    OrderByOperator(std::unique_ptr<IOperator> child, std::vector<OrderSpec> Order_specs);

    Batch* next() override;

    const Schema& get_schema() const override;
};

class LimitOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    int64_t cap_;

public:
    LimitOperator(std::unique_ptr<IOperator> child, int64_t cap);

    Batch* next() override;

    const Schema& get_schema() const override;
};

}  // namespace columnar
