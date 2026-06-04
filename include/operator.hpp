#pragma once

#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>
#include <absl/hash/hash.h>
#include <cstdint>
#include <memory>

#include "aggregate.hpp"
#include "batch.hpp"
#include "columnar_reader.hpp"
#include "expression.hpp"

namespace columnar {

class IOperator {
protected:
    Schema global_table_;

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

class GroupByAggOperator : public IOperator {
    struct GroupKey {
        absl::InlinedVector<int64_t, 2> values;

        bool operator==(const GroupKey& other) const {
            return values == other.values;
        }
    };

    struct KeyHash {
        size_t operator()(const GroupKey& key) const {
            size_t seed = 0;
            for (const auto& v : key.values) {
                seed ^=
                    absl::Hash<int64_t>{}(v) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
            }

            return seed;
        }
    };

    int64_t encode(const std::string& s) {
        auto it = str_to_id_.find(s);
        if (it != str_to_id_.end()) {
            return it->second;
        }

        int64_t id = static_cast<int64_t>(id_to_str_.size());
        id_to_str_.push_string(s);
        str_to_id_.emplace(id_to_str_.get_string(id_to_str_.size() - 1), id);

        return id;
    }

    std::string_view decode(int64_t id) const {
        return id_to_str_.get_string_view(static_cast<size_t>(id));
    }

private:
    std::unique_ptr<IOperator> child_;
    std::vector<std::unique_ptr<IValueExpression>> group_exprs_;
    std::vector<AggSpec> specs_;

    absl::flat_hash_map<GroupKey, std::vector<AggState>, KeyHash> groups_;

    absl::flat_hash_map<std::string, int64_t> str_to_id_;
    Column id_to_str_;

    Batch result_batch_;
    Schema result_schema_;
    bool done_ = false;

public:
    GroupByAggOperator(std::unique_ptr<IOperator>&& child,
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
    size_t global_offset_ = 0;
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

class HavingOperator : public IOperator {
private:
    std::unique_ptr<IOperator> child_;
    std::unique_ptr<IFilterExpression> filter_;

public:
    HavingOperator(std::unique_ptr<IOperator> child, std::unique_ptr<IFilterExpression> filter);

    Batch* next() override;

    const Schema& get_schema() const override;

private:
    static void apply_mask(Batch* batch, std::vector<uint8_t>& mask);
};

}  // namespace columnar
