#pragma once

#include <memory>
#include <variant>

#include "batch.hpp"
#include "column.hpp"

namespace columnar {

class IValueExpression {
public:
    virtual Column evaluate(const Batch& batch) = 0;
    virtual ~IValueExpression() = default;
};

class ColumnRef : public IValueExpression {
private:
    std::string name_;

public:
    ColumnRef(std::string name);

    Column evaluate(const Batch& batch) override;

    std::string name() const;
};

class Literal : public IValueExpression {
private:
    std::variant<int64_t, std::string> literal_;

public:
    template <typename T>
    Literal(T literal) : literal_(literal) {
    }

    Column evaluate(const Batch& batch) override;
};

class IFilterExpression {
public:
    virtual void evaluate(const Batch& batch, std::vector<uint8_t>& mask) = 0;
    virtual ~IFilterExpression() = default;
};

class NotEqual : public IFilterExpression {
private:
    std::unique_ptr<IValueExpression> left_;
    std::unique_ptr<IValueExpression> right_;

public:
    NotEqual(std::unique_ptr<ColumnRef>&& left, std::unique_ptr<Literal>&& right);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;

    template <typename T>
    void dispatch_int_comparator(const Column& left, const Column& right,
                                 std::vector<uint8_t>& mask) const {
        switch (right.type()) {
        case Type::Int16:
            compare<T, int16_t>(left, right, mask);
            break;
        case Type::Int32:
            compare<T, int32_t>(left, right, mask);
            break;
        case Type::Int64:
            compare<T, int64_t>(left, right, mask);
            break;
        case Type::Date:
            compare<T, int32_t>(left, right, mask);
            break;
        case Type::Timestamp:
            compare<T, int64_t>(left, right, mask);
            break;
        default:
            // wrong type
            break;
        }
    }

    template <typename T, typename F>
    void compare(const Column& left, const Column& right, std::vector<uint8_t>& mask) const {
        for (size_t i = 0; i < left.size(); ++i) {
            if (left.data_as<T>()[i] == right.data_as<F>()[i]) {
                mask[i] = 0;
            }
        }
    }

    static void dispatch_string_comparator(const Column& left, const Column& right,
                                           std::vector<uint8_t>& mask);
};

}  // namespace columnar
