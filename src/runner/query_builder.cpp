#include <memory>
#include <string>
#include <vector>

#include "src/execution/expression.hpp"
#include "src/execution/operator.hpp"
#include "src/runner/query_builder.hpp"

namespace columnar {

QueryBuilder::QueryBuilder(const std::string& path) : path_(path) {
}

std::unique_ptr<IOperator> QueryBuilder::scan(const std::vector<std::string>& cols) {
    return std::make_unique<ScanOperator>(path_, cols);
}

std::unique_ptr<IOperator> QueryBuilder::global_agg(std::unique_ptr<IOperator>&& child,
                                                    std::vector<AggSpec> specs) {
    return std::make_unique<GlobalAggOperator>(std::move(child), std::move(specs));
}

std::unique_ptr<IOperator> QueryBuilder::filter(std::unique_ptr<IOperator>&& child,
                                                std::unique_ptr<IFilterExpression>&& expr) {
    return std::make_unique<FilterOperator>(std::move(child), std::move(expr));
}

std::unique_ptr<IOperator>
QueryBuilder::group_by(std::unique_ptr<IOperator>&& child,
                       std::vector<std::unique_ptr<IValueExpression>>&& keys,
                       std::vector<AggSpec> aggs) {
    return std::make_unique<GroupByAggOperator>(std::move(child), std::move(keys), std::move(aggs));
}

std::unique_ptr<IOperator> QueryBuilder::order_by(std::unique_ptr<IOperator>&& child,
                                                  std::vector<OrderByOperator::OrderSpec> specs) {
    return std::make_unique<OrderByOperator>(std::move(child), std::move(specs));
}

std::unique_ptr<IOperator> QueryBuilder::limit(std::unique_ptr<IOperator>&& child, int64_t n,
                                               size_t offset) {
    return std::make_unique<LimitOperator>(std::move(child), n, offset);
}

std::unique_ptr<IOperator> QueryBuilder::top_k(std::unique_ptr<IOperator>&& child,
                                               std::vector<TopKOperator::OrderSpec> specs,
                                               int64_t n, size_t offset) {
    return std::make_unique<TopKOperator>(std::move(child), std::move(specs), n, offset);
}

std::unique_ptr<IOperator> QueryBuilder::having(std::unique_ptr<IOperator>&& child,
                                                std::unique_ptr<IFilterExpression>&& filter) {
    return std::make_unique<HavingOperator>(std::move(child), std::move(filter));
}

std::unique_ptr<ColumnRef> col(const std::string& name, std::unique_ptr<IValueExpression>&& expr) {
    return std::make_unique<ColumnRef>(name, std::move(expr));
}

std::unique_ptr<Add> add(std::unique_ptr<IValueExpression>&& left,
                         std::unique_ptr<IValueExpression>&& right) {
    return std::make_unique<Add>(std::move(left), std::move(right));
}

std::unique_ptr<Sub> sub(std::unique_ptr<IValueExpression>&& left,
                         std::unique_ptr<IValueExpression>&& right) {
    return std::make_unique<Sub>(std::move(left), std::move(right));
}

std::unique_ptr<Extract> extract(std::unique_ptr<IValueExpression>&& target,
                                 Extract::ExtractSpec spec) {
    return std::make_unique<Extract>(std::move(target), spec);
}

std::unique_ptr<StrLen> str_len(std::unique_ptr<IValueExpression>&& target) {
    return std::make_unique<StrLen>(std::move(target));
}

std::unique_ptr<DateTrunc> date_trunc(std::unique_ptr<IValueExpression>&& target,
                                      DateTrunc::TruncSpec spec) {
    return std::make_unique<DateTrunc>(std::move(target), spec);
}

std::unique_ptr<RegexpReplace> regexp_replace(std::unique_ptr<IValueExpression>&& target,
                                              const std::string& pattern,
                                              const std::string& replacement) {
    return std::make_unique<RegexpReplace>(std::move(target), pattern, replacement);
}

std::unique_ptr<CaseWhen> case_when(std::unique_ptr<IFilterExpression>&& condition,
                                    std::unique_ptr<IValueExpression>&& then,
                                    std::unique_ptr<IValueExpression>&& els) {
    return std::make_unique<CaseWhen>(std::move(condition), std::move(then), std::move(els));
}

std::unique_ptr<Compare> equal(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r) {
    return std::make_unique<Compare>(std::move(l), std::move(r), Compare::Cmp::E);
}

std::unique_ptr<Compare> not_equal(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r) {
    return std::make_unique<Compare>(std::move(l), std::move(r), Compare::Cmp::NE);
}

std::unique_ptr<Compare> greater(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r) {
    return std::make_unique<Compare>(std::move(l), std::move(r), Compare::Cmp::G);
}

std::unique_ptr<Compare> greater_equal(std::unique_ptr<ColumnRef>&& l,
                                       std::unique_ptr<Literal>&& r) {
    return std::make_unique<Compare>(std::move(l), std::move(r), Compare::Cmp::GE);
}

std::unique_ptr<Compare> less(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r) {
    return std::make_unique<Compare>(std::move(l), std::move(r), Compare::Cmp::L);
}

std::unique_ptr<Compare> less_equal(std::unique_ptr<ColumnRef>&& l, std::unique_ptr<Literal>&& r) {
    return std::make_unique<Compare>(std::move(l), std::move(r), Compare::Cmp::LE);
}

std::unique_ptr<And> and_expr(std::unique_ptr<IFilterExpression>&& left,
                              std::unique_ptr<IFilterExpression>&& right) {
    return std::make_unique<And>(std::move(left), std::move(right));
}

std::unique_ptr<In> in(std::unique_ptr<IValueExpression>&& column, const std::vector<int>& set) {
    return std::make_unique<In>(std::move(column), set);
}

std::unique_ptr<Like> like(std::unique_ptr<ColumnRef>&& l, const std::string& str) {
    return std::make_unique<Like>(std::move(l), str);
}

std::unique_ptr<NotLike> not_like(std::unique_ptr<ColumnRef>&& l, const std::string& str) {
    return std::make_unique<NotLike>(std::move(l), str);
}

AggSpec count(const std::string& col, std::string alias, std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::Count, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec count_distinct(const std::string& col, std::string alias,
                       std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::CountDistinct, col, std::make_unique<ColumnRef>(col, std::move(expr)),
                   alias);
}

AggSpec str_count_distinct(const std::string& col, std::string alias,
                           std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::StrCountDistinct, col,
                   std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec sum(const std::string& col, std::string alias, std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::Sum, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec min(const std::string& col, std::string alias, std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::Min, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec str_min(const std::string& col, std::string alias,
                std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::StrMin, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec max(const std::string& col, std::string alias, std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::Max, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec str_max(const std::string& col, std::string alias,
                std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::StrMax, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

AggSpec avg(const std::string& col, std::string alias, std::unique_ptr<IValueExpression>&& expr) {
    alias = (alias == "") ? col : alias;
    return AggSpec(AggType::Avg, col, std::make_unique<ColumnRef>(col, std::move(expr)), alias);
}

}  // namespace columnar
