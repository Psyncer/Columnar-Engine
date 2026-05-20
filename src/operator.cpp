#include <cstddef>
#include <numeric>
#include <utility>

#include "aggregate.hpp"
#include "batch.hpp"
#include "columnar_reader.hpp"
#include "data_type.hpp"
#include "operator.hpp"
#include "schema.hpp"

namespace columnar {

ScanOperator::ScanOperator(const std::string& path, const std::vector<std::string>& columns_needed)
    : reader_(ColumnarReader::open_output(path)), schema_(reader_.schema()),
      batch_(schema_, columns_needed) {
}

Batch* ScanOperator::next() {
    reader_.fill_batch(batch_);
    if (batch_.row_count_ == 0) {
        return nullptr;
    }

    batch_.active_rows_.resize(batch_.row_count_);
    std::iota(batch_.active_rows_.begin(), batch_.active_rows_.end(), 0);

    return &batch_;
}

const Schema& ScanOperator::get_schema() const {
    return schema_;
}

GlobalAggOperator::GlobalAggOperator(std::unique_ptr<IOperator>&& child, std::vector<AggSpec> specs)
    : child_(std::move(child)), specs_(std::move(specs)), states_(specs_.size()) {
    std::vector<std::string> needed_columns;
    for (const auto& spec : specs_) {
        result_schema_.add_column(spec.name, Type::Int64);
        needed_columns.push_back(spec.name);
    }
    result_batch_ = Batch(result_schema_, needed_columns);
}

Batch* GlobalAggOperator::next() {
    if (done_) {
        return nullptr;
    }
    done_ = true;

    while (Batch* batch = child_->next()) {
        for (size_t i = 0; i < specs_.size(); ++i) {
            const AggSpec& spec = specs_[i];
            AggState& state = states_[i];

            switch (spec.type) {
            case AggType::Count: {
                AggCount{}(batch->row_count_, state);
                break;
            }
            case AggType::CountDistinct: {
                dispatch_agg<AggCountDistinct>(*batch, state, spec.expr);
                break;
            }
            case AggType::Sum: {
                dispatch_agg<AggSum>(*batch, state, spec.expr);
                break;
            }
            case AggType::Min: {
                dispatch_agg<AggMin>(*batch, state, spec.expr);
                break;
            }
            case AggType::Max: {
                dispatch_agg<AggMax>(*batch, state, spec.expr);
                break;
            }
            case AggType::Avg: {
                AggCount{}(batch->row_count_, state);
                dispatch_agg<AggSum>(*batch, state, spec.expr);
                break;
            }
            }
        }
    }

    Schema result_schema;
    std::vector<std::string> needed_columns;
    for (const auto& spec : specs_) {
        result_schema.add_column(spec.name, Type::Int64);
        needed_columns.push_back(spec.name);
    }

    result_batch_ = Batch(result_schema, needed_columns);
    result_batch_.row_count_ = 1;

    for (size_t i = 0; i < specs_.size(); ++i) {
        states_[i].finalize(result_batch_.columns_[i], specs_[i].type);
    }

    return &result_batch_;
}

const Schema& GlobalAggOperator::get_schema() const {
    return result_schema_;
}

FilterOperator::FilterOperator(std::unique_ptr<IOperator>&& child,
                               std::unique_ptr<IFilterExpression>&& filter)
    : child_(std::move(child)), filter_(std::move(filter)) {
}

Batch* FilterOperator::next() {
    Batch* batch = child_->next();
    if (batch == nullptr) {
        return nullptr;
    }
    if (batch->row_count_ == 0) {
        return nullptr;
    }

    std::vector<uint8_t> mask(batch->row_count_, 1);
    filter_->evaluate(*batch, mask);
    apply_mask(batch, mask);

    return batch;
}

const Schema& FilterOperator::get_schema() const {
    return child_->get_schema();
}

void FilterOperator::apply_mask(Batch* batch, std::vector<uint8_t>& mask) {
    size_t new_row_count = 0;
    batch->active_rows_.clear();
    for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i] == 1) {
            batch->active_rows_.push_back(i);
            new_row_count++;
        }
    }
    batch->row_count_ = new_row_count;
}

}  // namespace columnar