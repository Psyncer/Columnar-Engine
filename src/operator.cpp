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
    : child_(std::move(child)), specs_(std::move(specs)), states_(specs_.size()) {
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

    while (Batch* batch = child_->next()) {
        for (size_t i = 0; i < specs_.size(); ++i) {
            const AggSpec& spec = specs_[i];
            AggState& state = states_[i];

            switch (spec.agg_type) {
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
                dispatch_agg<AggAvg>(*batch, state, spec.expr);
                break;
            }
            }
        }
    }

    Schema result_schema;
    std::vector<std::string> needed_columns;
    const Schema& schema = child_->get_schema();
    for (const auto& spec : specs_) {
        if (schema.get_column_type(spec.name) == Type::String) {
            result_schema.add_column(spec.output_name, Type::String);
        } else if (spec.agg_type == AggType::Min || spec.agg_type == AggType::Max ||
                   spec.agg_type == AggType::Avg) {
            result_schema.add_column(spec.output_name, schema.get_column_type(spec.name));
        } else {
            result_schema.add_column(spec.output_name, Type::Int64);
        }
        needed_columns.push_back(spec.output_name);
    }

    result_batch_ = Batch(result_schema, needed_columns);
    result_batch_.row_count_ = 1;
    result_batch_.active_rows_.push_back(0);

    for (size_t i = 0; i < specs_.size(); ++i) {
        states_[i].finalize(result_batch_.columns_[i], specs_[i].agg_type);
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

GroupByOperator::GroupByOperator(std::unique_ptr<IOperator>&& child,
                                 std::vector<std::unique_ptr<IValueExpression>> group_exprs,
                                 std::vector<AggSpec> specs)
    : child_(std::move(child)), group_exprs_(std::move(group_exprs)), specs_(std::move(specs)) {
}

Batch* GroupByOperator::next() {
    if (done_) {
        return nullptr;
    }
    done_ = true;

    while (Batch* batch = child_->next()) {
        for (const auto& row : batch->active_rows_) {
            GroupKey key;
            for (const auto& expr : group_exprs_) {
                Column column = expr->evaluate(*batch);
                switch (column.type()) {
                case Type::Int16:
                    key.ints.push_back(static_cast<int64_t>(column.get_value<int16_t>(row)));
                    break;
                case Type::Int32:
                    key.ints.push_back(static_cast<int64_t>(column.get_value<int32_t>(row)));
                    break;
                case Type::Int64:
                    key.ints.push_back(static_cast<int64_t>(column.get_value<int64_t>(row)));
                    break;
                case Type::String:
                    key.strs.push_back(column.get_string(row));
                    break;
                case Type::Date:
                    key.ints.push_back(static_cast<int64_t>(column.get_value<int32_t>(row)));
                    break;
                case Type::Timestamp:
                    key.ints.push_back(static_cast<int64_t>(column.get_value<int64_t>(row)));
                    break;
                }
            }

            std::vector<AggState>& states = groups_[key];

            if (states.empty()) {
                states.resize(specs_.size());
            }

            for (size_t i = 0; i < specs_.size(); ++i) {
                AggState& state = states[i];
                AggSpec& spec = specs_[i];
                switch (specs_[i].agg_type) {
                case AggType::Count: {
                    state.count++;
                    break;
                }
                case AggType::CountDistinct: {
                    Column column = spec.expr->evaluate(*batch);
                    switch (column.type()) {
                    case Type::Int16:
                        state.distinct.insert(static_cast<int64_t>(column.get_value<int16_t>(row)));
                        break;
                    case Type::Int32:
                        state.distinct.insert(static_cast<int64_t>(column.get_value<int32_t>(row)));
                        break;
                    case Type::Int64:
                        state.distinct.insert(static_cast<int64_t>(column.get_value<int64_t>(row)));
                        break;
                    case Type::String:
                        state.str_distinct.insert(column.get_string(row));
                        break;
                    case Type::Date:
                        state.distinct.insert(static_cast<int64_t>(column.get_value<int32_t>(row)));
                        break;
                    case Type::Timestamp:
                        state.distinct.insert(static_cast<int64_t>(column.get_value<int64_t>(row)));
                        break;
                    }
                    break;
                }
                case AggType::Sum: {
                    Column column = spec.expr->evaluate(*batch);
                    switch (column.type()) {
                    case Type::Int16:
                        state.sum += static_cast<int64_t>(column.get_value<int16_t>(row));
                        break;
                    case Type::Int32:
                        state.sum += static_cast<int64_t>(column.get_value<int32_t>(row));
                        break;
                    case Type::Int64:
                        state.sum += static_cast<int64_t>(column.get_value<int64_t>(row));
                        break;
                    case Type::Date:
                        state.sum += static_cast<int64_t>(column.get_value<int32_t>(row));
                        break;
                    case Type::Timestamp:
                        state.sum += static_cast<int64_t>(column.get_value<int64_t>(row));
                        break;
                    default:
                        // wrong type
                        break;
                    }
                    break;
                }
                case AggType::Min: {
                    Column column = spec.expr->evaluate(*batch);
                    switch (column.type()) {
                    case Type::Int16: {
                        int64_t val = static_cast<int64_t>(column.get_value<int16_t>(row));
                        if (val < state.min) {
                            state.min = val;
                        }
                        break;
                    }
                    case Type::Int32: {
                        int64_t val = static_cast<int64_t>(column.get_value<int32_t>(row));
                        if (val < state.min) {
                            state.min = val;
                        }
                        break;
                    }
                    case Type::Int64: {
                        int64_t val = static_cast<int64_t>(column.get_value<int64_t>(row));
                        if (val < state.min) {
                            state.min = val;
                        }
                        break;
                    }
                    case Type::String: {
                        std::string str = column.get_string(row);
                        if (str < state.str_min) {
                            state.str_min = str;
                        }
                        break;
                    }
                    case Type::Date: {
                        int64_t val = static_cast<int64_t>(column.get_value<int32_t>(row));
                        if (val < state.min) {
                            state.min = val;
                        }
                        break;
                    }
                    case Type::Timestamp: {
                        int64_t val = static_cast<int64_t>(column.get_value<int64_t>(row));
                        if (val < state.min) {
                            state.min = val;
                        }
                        break;
                    }
                    }
                    break;
                }
                case AggType::Max: {
                    Column column = spec.expr->evaluate(*batch);
                    switch (column.type()) {
                    case Type::Int16: {
                        int64_t val = static_cast<int64_t>(column.get_value<int16_t>(row));
                        if (val > state.max) {
                            state.max = val;
                        }
                        break;
                    }
                    case Type::Int32: {
                        int64_t val = static_cast<int64_t>(column.get_value<int32_t>(row));
                        if (val > state.max) {
                            state.max = val;
                        }
                        break;
                    }
                    case Type::Int64: {
                        int64_t val = static_cast<int64_t>(column.get_value<int64_t>(row));
                        if (val > state.max) {
                            state.max = val;
                        }
                        break;
                    }
                    case Type::String: {
                        std::string str = column.get_string(row);
                        if (str > state.str_max) {
                            state.str_max = str;
                        }
                        break;
                    }
                    case Type::Date: {
                        int64_t val = static_cast<int64_t>(column.get_value<int32_t>(row));
                        if (val > state.max) {
                            state.max = val;
                        }
                        break;
                    }
                    case Type::Timestamp: {
                        int64_t val = static_cast<int64_t>(column.get_value<int64_t>(row));
                        if (val > state.max) {
                            state.max = val;
                        }
                        break;
                    }
                    }
                    break;
                }
                case AggType::Avg: {
                    Column column = spec.expr->evaluate(*batch);
                    state.avg.second++;
                    switch (column.type()) {
                    case Type::Int16:
                        state.avg.first += static_cast<int64_t>(column.get_value<int16_t>(row));
                        break;
                    case Type::Int32:
                        state.avg.first += static_cast<int64_t>(column.get_value<int32_t>(row));
                        break;
                    case Type::Int64:
                        state.avg.first += static_cast<int64_t>(column.get_value<int64_t>(row));
                        break;
                    case Type::Date:
                        state.avg.first += static_cast<int64_t>(column.get_value<int32_t>(row));
                        break;
                    case Type::Timestamp:
                        state.avg.first += static_cast<int64_t>(column.get_value<int64_t>(row));
                        break;
                    default:
                        // wrong type
                        break;
                    }
                    break;
                }
                }
            }
        }
    }

    std::vector<std::string> needed_columns;
    const Schema& schema = child_->get_schema();
    for (const auto& expr : group_exprs_) {
        ColumnRef* ref = dynamic_cast<ColumnRef*>(expr.get());
        std::string name = ref->name();
        result_schema_.add_column(name, schema.get_column_type(name));
        needed_columns.push_back(name);
    }

    for (const auto& spec : specs_) {
        if (schema.get_column_type(spec.name) == Type::String) {
            result_schema_.add_column(spec.output_name, Type::String);
        } else if (spec.agg_type == AggType::Min || spec.agg_type == AggType::Max) {
            result_schema_.add_column(spec.output_name, schema.get_column_type(spec.name));
        } else {
            result_schema_.add_column(spec.output_name, Type::Int64);
        }
        needed_columns.push_back(spec.output_name);
    }

    result_batch_ = Batch(result_schema_, needed_columns);

    for (auto& [key, state] : groups_) {
        size_t int_idx = 0;
        size_t str_idx = 0;

        for (size_t i = 0; i < group_exprs_.size(); ++i) {
            Column& column = result_batch_.columns_[i];
            if (column.type() == Type::String) {
                column.push_string(key.strs[str_idx++]);

            } else {
                column.push(key.ints[int_idx++]);
            }
        }

        for (size_t i = 0; i < specs_.size(); ++i) {
            state[i].finalize(result_batch_.columns_[group_exprs_.size() + i], specs_[i].agg_type);
        }

        result_batch_.active_rows_.push_back(result_batch_.row_count_);
        result_batch_.row_count_++;
    }

    return &result_batch_;
}

const Schema& GroupByOperator::get_schema() const {
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

    while (Batch* batch = child_->next()) {
        if (accumulated_batch_.columns_.empty()) {
            accumulated_batch_ =
                Batch(child_->get_schema(), child_->get_schema().get_column_names());
        }

        for (const auto& row : batch->active_rows_) {
            for (size_t i = 0; i < batch->column_count_; ++i) {
                switch (batch->columns_[i].type()) {
                case Type::Int16:
                    accumulated_batch_.columns_[i].push_value<int16_t>(
                        batch->columns_[i].get_value<int16_t>(row));
                    break;
                case Type::Int32:
                    accumulated_batch_.columns_[i].push_value<int32_t>(
                        batch->columns_[i].get_value<int32_t>(row));
                    break;
                case Type::Int64:
                    accumulated_batch_.columns_[i].push_value<int64_t>(
                        batch->columns_[i].get_value<int64_t>(row));
                    break;
                case Type::String:
                    accumulated_batch_.columns_[i].push_string(batch->columns_[i].get_string(row));
                    break;
                case Type::Date:
                    accumulated_batch_.columns_[i].push_value<int32_t>(
                        batch->columns_[i].get_value<int32_t>(row));
                    break;
                case Type::Timestamp:
                    accumulated_batch_.columns_[i].push_value<int64_t>(
                        batch->columns_[i].get_value<int64_t>(row));
                    break;
                }
            }
            accumulated_batch_.active_rows_.push_back(accumulated_batch_.row_count_);
            accumulated_batch_.row_count_++;
        }
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

}  // namespace columnar