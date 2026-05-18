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
    virtual Schema get_schema() const = 0;
};



class ScanOperator : public IOperator {
private:
    ColumnarReader reader_;
    Schema schema_;
    Batch batch_;

public:
    ScanOperator(const std::string& path, const std::vector<std::string>& columns_needed);

    Batch* next() override;

    Schema get_schema() const override;
};



class GlobalAggOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    Schema schema_;
    Batch result_batch_;
    std::vector<AggSpec> specs_;
    std::vector<AggState> states_;
    bool done_ = false;

public:
    GlobalAggOperator(std::unique_ptr<IOperator>&& child, std::vector<AggSpec> specs);

    Batch* next() override;

    Schema get_schema() const override;
};



class FilterOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    std::unique_ptr<IExpression> filter_;
    Batch result_batch_;

public:
    FilterOperator(std::unique_ptr<IOperator>&& child, std::unique_ptr<IExpression>&& filter);

    Batch* next() override;

    Schema get_schema() const override;

private:
    static void apply_mask(Batch* batch, std::vector<uint8_t>& mask);
};



class GroupAggOperator : public IOperator {
public:
    Batch* next() override;
};



class KMinOperator : public IOperator {
public:
    Batch* next() override;
};



class SortOperator : public IOperator {
public:
    Batch* next() override;
};

}  // namespace columnar
