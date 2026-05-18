#pragma once

#include <memory>
#include <variant>

#include "batch.hpp"
#include "column.hpp"

namespace columnar {

class ColumnRef {
private:
    std::string name_;

public:
    ColumnRef(std::string name);

    const Column* get_column(const Batch& batch) const;
};

class Literal {
private:
    std::variant<int64_t, std::string> literal_;

public:
    template <typename T>
    Literal(T literal) : literal_(literal) {
    }

    std::variant<int64_t, std::string> get_literal();
};

class IExpression {
public:
    virtual void evaluate(const Batch& batch, std::vector<uint8_t>& mask) = 0;
    virtual ~IExpression() = default;
};

class NotEqual : public IExpression {
private:
    std::unique_ptr<ColumnRef> left_;
    std::unique_ptr<Literal> right_;

public:
    NotEqual(std::unique_ptr<ColumnRef> left, std::unique_ptr<Literal> right);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;

    template <typename T>
    void dispatch_int_comparator(const Column* column, T value, std::vector<uint8_t>& mask) {
        for (size_t i = 0; i < column->size(); ++i) {
            if (column->data_as<T>()[i] == value) {
                mask[i] = 0;
            }
        }
    }

    static void dispatch_string_comparator(const Column* column, std::string str,
                                           std::vector<uint8_t>& mask);
};

}  // namespace columnar
