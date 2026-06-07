#pragma once

#include <memory>
#include <string>
#include <vector>

#include "src/execution/expression.hpp"
#include "src/execution/operator.hpp"

namespace columnar {

struct QueryBuilder {
    std::string path_;

    QueryBuilder(const std::string& path);

    std::unique_ptr<IOperator> scan(const std::vector<std::string>& cols);

    static std::unique_ptr<IOperator> global_agg(std::unique_ptr<IOperator>&& child,
                                                 std::vector<AggSpec> specs);

    static std::unique_ptr<IOperator> filter(std::unique_ptr<IOperator>&& child,
                                             std::unique_ptr<IFilterExpression>&& expr);

    static std::unique_ptr<IOperator>
    group_by(std::unique_ptr<IOperator>&& child,
             std::vector<std::unique_ptr<IValueExpression>>&& keys, std::vector<AggSpec> aggs);

    static std::unique_ptr<IOperator> order_by(std::unique_ptr<IOperator>&& child,
                                               std::vector<OrderByOperator::OrderSpec> specs);

    static std::unique_ptr<IOperator> limit(std::unique_ptr<IOperator>&& child, int64_t n,
                                            size_t offset = 0);

    static std::unique_ptr<IOperator> top_k(std::unique_ptr<IOperator>&& child,
                                            std::vector<TopKOperator::OrderSpec> specs, int64_t n,
                                            size_t offset = 0);

    static std::unique_ptr<IOperator> having(std::unique_ptr<IOperator>&& child,
                                             std::unique_ptr<IFilterExpression>&& filter);
};

std::unique_ptr<ColumnRef> col(const std::string& name,
                               std::unique_ptr<IValueExpression>&& expr = nullptr);

template <typename T>
std::unique_ptr<Literal> lit(T&& val) {
    return std::make_unique<Literal>(std::forward<T>(val));
}

std::unique_ptr<Add> add(std::unique_ptr<IValueExpression>&& left,
                         std::unique_ptr<IValueExpression>&& right);

std::unique_ptr<Sub> sub(std::unique_ptr<IValueExpression>&& left,
                         std::unique_ptr<IValueExpression>&& right);

std::unique_ptr<Extract> extract(std::unique_ptr<IValueExpression>&& target,
                                 Extract::ExtractSpec spec);

std::unique_ptr<StrLen> str_len(std::unique_ptr<IValueExpression>&& target);

std::unique_ptr<DateTrunc> date_trunc(std::unique_ptr<IValueExpression>&& target,
                                      DateTrunc::TruncSpec spec);

std::unique_ptr<RegexpReplace> regexp_replace(std::unique_ptr<IValueExpression>&& target,
                                              const std::string& pattern,
                                              const std::string& replacement);

std::unique_ptr<CaseWhen> case_when(std::unique_ptr<IFilterExpression>&& condition,
                                    std::unique_ptr<IValueExpression>&& then,
                                    std::unique_ptr<IValueExpression>&& els);

std::unique_ptr<Compare> equal(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r);

std::unique_ptr<Compare> not_equal(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r);

std::unique_ptr<Compare> greater(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r);

std::unique_ptr<Compare> greater_equal(std::unique_ptr<ColumnRef>&& l,
                                       std::unique_ptr<Literal>&& r);

std::unique_ptr<Compare> less(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r);

std::unique_ptr<Compare> less_equal(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r);

std::unique_ptr<And> and_expr(std::unique_ptr<IFilterExpression>&& left,
                              std::unique_ptr<IFilterExpression>&& right);

std::unique_ptr<In> in(std::unique_ptr<IValueExpression>&& column, const std::vector<int>& set);

std::unique_ptr<Like> like(std::unique_ptr<ColumnRef>&& l, const std::string& str);

std::unique_ptr<NotLike> not_like(std::unique_ptr<ColumnRef>&& l, const std::string& str);

AggSpec count(const std::string& col, std::string alias = "",
              std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec count_distinct(const std::string& col, std::string alias = "",
                       std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec str_count_distinct(const std::string& col, std::string alias = "",
                           std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec sum(const std::string& col, std::string alias = "",
            std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec min(const std::string& col, std::string alias = "",
            std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec str_min(const std::string& col, std::string alias = "",
                std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec max(const std::string& col, std::string alias = "",
            std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec str_max(const std::string& col, std::string alias = "",
                std::unique_ptr<IValueExpression>&& expr = nullptr);

AggSpec avg(const std::string& col, std::string alias = "",
            std::unique_ptr<IValueExpression>&& expr = nullptr);

template <typename... Ts>
std::vector<AggSpec> make_aggs(Ts&&... ts) {
    std::vector<AggSpec> v;
    v.reserve(sizeof...(Ts));

    (v.emplace_back(std::forward<Ts>(ts)), ...);

    return v;
}

template <typename... Ts>
std::vector<std::unique_ptr<IValueExpression>> make_groups(Ts&&... ts) {
    std::vector<std::unique_ptr<IValueExpression>> v;
    v.reserve(sizeof...(Ts));

    (v.emplace_back(std::forward<Ts>(ts)), ...);

    return v;
}

template <typename... Ts>
std::vector<OrderByOperator::OrderSpec> order_specs(Ts&&... ts) {
    std::vector<OrderByOperator::OrderSpec> v;
    v.reserve(sizeof...(Ts));

    (v.emplace_back(std::forward<Ts>(ts)), ...);

    return v;
}

template <typename... Ts>
std::vector<TopKOperator::OrderSpec> top_k_specs(Ts&&... ts) {
    std::vector<TopKOperator::OrderSpec> v;
    v.reserve(sizeof...(Ts));

    (v.emplace_back(std::forward<Ts>(ts)), ...);

    return v;
}

}  // namespace columnar
