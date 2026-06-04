#include <cstddef>
#include <cstdint>
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
    : child_(std::move(child)), specs_(std::move(specs)) {
    std::vector<std::string> needed_columns;
    for (const auto& spec : specs_) {
        result_schema_.add_column(spec.output_name, Type::Int64);
        needed_columns.push_back(spec.output_name);
    }
    result_batch_ = Batch(result_schema_, needed_columns);
}

Batch* GlobalAggOperator::next() {
    if (done_) {
        return nullptr;
    }
    done_ = true;

    bool states_initialized = false;

    while (Batch* batch = child_->next()) {
        if (!states_initialized) {
            states_.resize(specs_.size());
            for (size_t i = 0; i < specs_.size(); ++i) {
                initialize_state(states_[i].state, specs_[i].agg_type);
            }
            states_initialized = true;
        }

        for (size_t i = 0; i < specs_.size(); ++i) {
            Column column(specs_[i].expr->evaluate(*batch));

            switch (specs_[i].agg_type) {
            case AggType::Count:
                aggregate_count(batch->active_rows_.size(), std::get<CountState>(states_[i].state));
                break;
            case AggType::CountDistinct:
                aggregate_count_distinct(column, std::get<CountDistinctState>(states_[i].state));
                break;
            case AggType::StrCountDistinct:
                aggregate_str_count_distinct(column,
                                             std::get<StrCountDistinctState>(states_[i].state));
                break;
            case AggType::Sum:
                aggregate_sum(column, std::get<SumState>(states_[i].state));
                break;
            case AggType::Min:
                aggregate_min(column, std::get<MinState>(states_[i].state));
                break;
            case AggType::StrMin:
                aggregate_str_min(column, std::get<StrMinState>(states_[i].state));
                break;
            case AggType::Max:
                aggregate_max(column, std::get<MaxState>(states_[i].state));
                break;
            case AggType::StrMax:
                aggregate_str_max(column, std::get<StrMaxState>(states_[i].state));
                break;
            case AggType::Avg:
                aggregate_avg(column, std::get<AvgState>(states_[i].state));
                break;
            }
        }
    }

    Schema result_schema;
    std::vector<std::string> needed_columns;
    for (const auto& spec : specs_) {
        if (spec.agg_type == AggType::Count || spec.agg_type == AggType::CountDistinct ||
            spec.agg_type == AggType::Sum || spec.agg_type == AggType::Avg) {
            result_schema.add_column(spec.output_name, Type::Int64);
        } else {
            result_schema.add_column(spec.output_name, spec.expr->output_type());
        }
        needed_columns.push_back(spec.output_name);
    }

    result_batch_ = Batch(result_schema, needed_columns);
    result_batch_.row_count_ = 1;
    result_batch_.active_rows_.push_back(0);

    for (size_t i = 0; i < specs_.size(); ++i) {
        states_[i].finalize(result_batch_.columns_[i]);
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
    batch->active_rows_.clear();
    for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i] == 1) {
            batch->active_rows_.push_back(i);
        }
    }
}

GroupByAggOperator::GroupByAggOperator(std::unique_ptr<IOperator>&& child,
                                       std::vector<std::unique_ptr<IValueExpression>> group_exprs,
                                       std::vector<AggSpec> specs)
    : child_(std::move(child)), group_exprs_(std::move(group_exprs)), specs_(std::move(specs)),
      id_to_str_(Type::String, -1) {
    str_to_id_.reserve(1 << 20);
    groups_.reserve(1 << 20);
}

Batch* GroupByAggOperator::next() {
    if (done_) {
        return nullptr;
    }
    done_ = true;

    int debug_entry = 0;

    while (Batch* batch = child_->next()) {

        if (debug_entry == 0) {
            std::cout << "ENTERED GROUP BY LOOP" << std::endl;
            debug_entry = 1;
        }

        std::vector<Column> columns;
        columns.reserve(group_exprs_.size());

        for (const auto& expr : group_exprs_) {
            columns.emplace_back(expr->evaluate(*batch));
        }

        std::vector<Column> agg_cols;
        agg_cols.reserve(specs_.size());
        for (const auto& spec : specs_) {
            agg_cols.emplace_back(spec.expr->evaluate(*batch));
        }

        std::string str;
        int64_t id;
        GroupKey key;
        key.values.reserve(columns.size());

        for (const auto& row : batch->active_rows_) {
            key.values.clear();
            for (const auto& column : columns) {
                switch (column.type()) {
                case Type::Int16:
                    key.values.push_back(static_cast<int64_t>(column.get_value<int16_t>(row)));
                    break;
                case Type::Int32:
                    key.values.push_back(static_cast<int64_t>(column.get_value<int32_t>(row)));
                    break;
                case Type::Int64:
                    key.values.push_back(static_cast<int64_t>(column.get_value<int64_t>(row)));
                    break;
                case Type::String:
                    str = column.get_string(row);
                    id = encode(str);
                    key.values.push_back(id);
                    break;
                case Type::Date:
                    key.values.push_back(static_cast<int64_t>(column.get_value<int32_t>(row)));
                    break;
                case Type::Timestamp:
                    key.values.push_back(static_cast<int64_t>(column.get_value<int64_t>(row)));
                    break;
                }
            }

            std::vector<AggState>& states = groups_[key];

            if (states.empty()) {
                states.resize(specs_.size());
                for (size_t i = 0; i < specs_.size(); ++i) {
                    initialize_state(states[i].state, specs_[i].agg_type);
                }
            }

            for (size_t i = 0; i < specs_.size(); ++i) {
                const Column& column = agg_cols[i];
                switch (specs_[i].agg_type) {
                case AggType::Count:
                    aggregate_count(batch->active_rows_.size(),
                                    std::get<CountState>(states[i].state));
                    break;
                case AggType::CountDistinct:
                    aggregate_count_distinct(column, std::get<CountDistinctState>(states[i].state),
                                             row);
                    break;
                case AggType::StrCountDistinct:
                    aggregate_str_count_distinct(
                        column, std::get<StrCountDistinctState>(states[i].state), row);
                    break;
                case AggType::Sum:
                    aggregate_sum(column, std::get<SumState>(states[i].state), row);
                    break;
                case AggType::Min:
                    aggregate_min(column, std::get<MinState>(states[i].state), row);
                    break;
                case AggType::StrMin:
                    aggregate_str_min(column, std::get<StrMinState>(states[i].state), row);
                    break;
                case AggType::Max:
                    aggregate_max(column, std::get<MaxState>(states[i].state), row);
                    break;
                case AggType::StrMax:
                    aggregate_str_max(column, std::get<StrMaxState>(states[i].state), row);
                    break;
                case AggType::Avg:
                    aggregate_avg(column, std::get<AvgState>(states[i].state), row);
                    break;
                }
            }
        }
    }

    std::vector<std::string> needed_columns;
    for (const auto& expr : group_exprs_) {
        ColumnRef* ref = dynamic_cast<ColumnRef*>(expr.get());
        std::string name = ref->name();
        Type type = ref->output_type();
        result_schema_.add_column(name, type);
        needed_columns.push_back(name);
    }

    for (const auto& spec : specs_) {
        if (spec.agg_type == AggType::Count || spec.agg_type == AggType::CountDistinct ||
            spec.agg_type == AggType::Sum || spec.agg_type == AggType::Avg) {
            result_schema_.add_column(spec.output_name, Type::Int64);
        } else {
            result_schema_.add_column(spec.output_name, spec.expr->output_type());
        }
        needed_columns.push_back(spec.output_name);
    }

    result_batch_ = Batch(result_schema_, needed_columns);

    for (const auto& [key, state] : groups_) {
        for (size_t i = 0; i < group_exprs_.size(); ++i) {
            Column& column = result_batch_.columns_[i];
            switch (column.type()) {
            case Type::Int16:
                column.push_value<int16_t>(key.values[i]);
                break;
            case Type::Int32:
                column.push_value<int32_t>(key.values[i]);
                break;
            case Type::Int64:
                column.push_value<int64_t>(key.values[i]);
                break;
            case Type::String:
                column.push_string(std::string(decode(key.values[i])));
                break;
            case Type::Date:
                column.push_value<int32_t>(key.values[i]);
                break;
            case Type::Timestamp:
                column.push_value<int64_t>(key.values[i]);
                break;
            }
        }

        for (size_t i = 0; i < specs_.size(); ++i) {
            state[i].finalize(result_batch_.columns_[group_exprs_.size() + i]);
        }

        result_batch_.active_rows_.push_back(result_batch_.row_count_);
        result_batch_.row_count_++;
    }

    return &result_batch_;
}

const Schema& GroupByAggOperator::get_schema() const {
    return result_schema_;
}

OrderByOperator::OrderByOperator(std::unique_ptr<IOperator> child,
                                 std::vector<OrderSpec> order_specs)
    : child_(std::move(child)), order_specs_(std::move(order_specs)) {
}

Batch* OrderByOperator::next() {
    if (done_) {
        return nullptr;
    }
    done_ = true;

    int debug_entry = 0;

    while (Batch* batch = child_->next()) {

        if (debug_entry == 0) {
            std::cout << "ENTERED ORDER BY" << std::endl;
            debug_entry = 1;
        }

        if (accumulated_batch_.columns_.empty()) {
            accumulated_batch_ = Batch(*batch);
            global_offset_ = accumulated_batch_.columns_[0].size();
            continue;
        }

        for (size_t i = 0; i < batch->column_count_; ++i) {
            switch (accumulated_batch_.columns_[i].type()) {
            case Type::Int16:
                accumulated_batch_.columns_[i].emplace_column<int16_t>(
                    batch->columns_[i].data(), batch->columns_[i].size() * sizeof(int16_t));
                break;
            case Type::Int32:
                accumulated_batch_.columns_[i].emplace_column<int32_t>(
                    batch->columns_[i].data(), batch->columns_[i].size() * sizeof(int32_t));
                break;
            case Type::Int64:
                accumulated_batch_.columns_[i].emplace_column<int64_t>(
                    batch->columns_[i].data(), batch->columns_[i].size() * sizeof(int64_t));
                break;
            case Type::String:
                for (size_t row = 0; row < batch->columns_[i].size(); ++row) {
                    accumulated_batch_.columns_[i].push_string(batch->columns_[i].get_string(row));
                }
                break;
            case Type::Date:
                accumulated_batch_.columns_[i].emplace_column<int32_t>(
                    batch->columns_[i].data(), batch->columns_[i].size() * sizeof(int32_t));
                break;
            case Type::Timestamp:
                accumulated_batch_.columns_[i].emplace_column<int64_t>(
                    batch->columns_[i].data(), batch->columns_[i].size() * sizeof(int64_t));
                break;
            }
        }

        for (const auto& row : batch->active_rows_) {
            accumulated_batch_.active_rows_.push_back(row + global_offset_);
        }
        accumulated_batch_.row_count_ += batch->row_count_;
        global_offset_ = accumulated_batch_.columns_[0].size();
    }

    std::vector<size_t>& indices = accumulated_batch_.active_rows_;

    std::vector<Column> order_columns;
    order_columns.reserve(order_specs_.size());
    for (const auto& spec : order_specs_) {
        order_columns.push_back(spec.expr->evaluate(accumulated_batch_));
    }

    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        for (size_t i = 0; i < order_specs_.size(); ++i) {
            const auto& spec = order_specs_[i];
            const Column& column = order_columns[i];
            switch (column.type()) {
            case Type::Int16: {
                int16_t va = column.get_value<int16_t>(a);
                int16_t vb = column.get_value<int16_t>(b);
                if (va != vb) {
                    return spec.dir == OrderDirection::Asc ? va < vb : va > vb;
                }
                break;
            }
            case Type::Int32: {
                int32_t va = column.get_value<int32_t>(a);
                int32_t vb = column.get_value<int32_t>(b);
                if (va != vb) {
                    return spec.dir == OrderDirection::Asc ? va < vb : va > vb;
                }
                break;
            }
            case Type::Int64: {
                int64_t va = column.get_value<int64_t>(a);
                int64_t vb = column.get_value<int64_t>(b);
                if (va != vb) {
                    return spec.dir == OrderDirection::Asc ? va < vb : va > vb;
                }
                break;
            }
            case Type::String: {
                std::string stra = column.get_string(a);
                std::string strb = column.get_string(b);
                if (stra != strb) {
                    return spec.dir == OrderDirection::Asc ? stra < strb : stra > strb;
                }
                break;
            }
            case Type::Date: {
                int32_t va = column.get_value<int32_t>(a);
                int32_t vb = column.get_value<int32_t>(b);
                if (va != vb) {
                    return spec.dir == OrderDirection::Asc ? va < vb : va > vb;
                }
                break;
            }
            case Type::Timestamp: {
                int64_t va = column.get_value<int64_t>(a);
                int64_t vb = column.get_value<int64_t>(b);
                if (va != vb) {
                    return spec.dir == OrderDirection::Asc ? va < vb : va > vb;
                }
                break;
            }
            }
        }
        return false;
    });

    return &accumulated_batch_;
}

const Schema& OrderByOperator::get_schema() const {
    return child_->get_schema();
}

LimitOperator::LimitOperator(std::unique_ptr<IOperator> child, size_t len, size_t offset)
    : child_(std::move(child)), len_(len), offset_(offset) {
}

Batch* LimitOperator::next() {
    while (len_ > 0) {
        Batch* batch = child_->next();

        if (batch == nullptr) {
            return nullptr;
        }

        std::vector<size_t> new_active_rows;

        for (size_t row : batch->active_rows_) {
            if (offset_ > 0) {
                offset_--;
                continue;
            }

            if (len_ == 0) {
                break;
            }

            new_active_rows.push_back(row);
            len_--;
        }

        if (!new_active_rows.empty()) {
            batch->active_rows_ = std::move(new_active_rows);
            return batch;
        }
    }

    return nullptr;
}

const Schema& LimitOperator::get_schema() const {
    return child_->get_schema();
}

HavingOperator::HavingOperator(std::unique_ptr<IOperator> child,
                               std::unique_ptr<IFilterExpression> filter)
    : child_(std::move(child)), filter_(std::move(filter)) {
}

Batch* HavingOperator::next() {
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

void HavingOperator::apply_mask(Batch* batch, std::vector<uint8_t>& mask) {
    batch->active_rows_.clear();
    for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i] == 1) {
            batch->active_rows_.push_back(i);
        }
    }
}

const Schema& HavingOperator::get_schema() const {
    return child_->get_schema();
}

}  // namespace columnar