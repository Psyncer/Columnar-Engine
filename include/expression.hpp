#pragma once

#include <memory>
#include <variant>

#include "batch.hpp"
#include "column.hpp"

namespace columnar {

class IValueExpression {
public:
    virtual const Column* evaluate(const Batch& batch) = 0;
    virtual ~IValueExpression() = default;
};

class ColumnRef : public IValueExpression {
private:
    std::string name_;
    const Column* column_;

public:
    ColumnRef(std::string name);

    const Column* evaluate(const Batch& batch) override;

    std::string name() const;
};

class Literal : public IValueExpression {
private:
    std::variant<int64_t, std::string> literal_;
    Column column_;

public:
    template <typename T>
    Literal(T literal) : literal_(literal), column_(Type::Int16, 0) {
    }

    const Column* evaluate(const Batch& batch) override;
};

class Add : public IValueExpression {
private:
    std::unique_ptr<IValueExpression> left_;
    std::unique_ptr<IValueExpression> right_;
    Column column_;

public:
    Add(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right);

    const Column* evaluate(const Batch& batch) override;
};

class Sub : public IValueExpression {
private:
    std::unique_ptr<IValueExpression> left_;
    std::unique_ptr<IValueExpression> right_;
    Column column_;

public:
    Sub(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right);

    const Column* evaluate(const Batch& batch) override;
};

class IFilterExpression {
public:
    virtual void evaluate(const Batch& batch, std::vector<uint8_t>& mask) = 0;
    virtual ~IFilterExpression() = default;
};

class Compare : public IFilterExpression {
public:
    enum class Cmp {
        NE,
        E,
        GE,
        LE,
        G,
        L,
    };

private:
    std::unique_ptr<IValueExpression> left_;
    std::unique_ptr<IValueExpression> right_;
    Cmp cmp_;

public:
    Compare(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right,
            Cmp cmp);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;

    template <typename T>
    void dispatch_int_comparator(const Column& left, const Column& right,
                                 std::vector<uint8_t>& mask) const {
        switch (right.type()) {
        case Type::Int16:
            compare<T, int16_t>(left, right, mask, cmp_);
            break;
        case Type::Int32:
            compare<T, int32_t>(left, right, mask, cmp_);
            break;
        case Type::Int64:
            compare<T, int64_t>(left, right, mask, cmp_);
            break;
        case Type::Date:
            compare<T, int32_t>(left, right, mask, cmp_);
            break;
        case Type::Timestamp:
            compare<T, int64_t>(left, right, mask, cmp_);
            break;
        default:
            // wrong type
            break;
        }
    }

    template <typename T, typename F>
    void compare(const Column& left, const Column& right, std::vector<uint8_t>& mask,
                 Cmp cmp) const {
        switch (cmp) {
        case Cmp::NE:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] == right.data_as<F>()[i]) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::E:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] != right.data_as<F>()[i]) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::GE:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] < right.data_as<F>()[i]) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::LE:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] > right.data_as<F>()[i]) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::G:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] <= right.data_as<F>()[i]) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::L:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] >= right.data_as<F>()[i]) {
                    mask[i] = 0;
                }
            }
            break;
        }
    }

    static void dispatch_string_comparator(const Column& left, const Column& right,
                                           std::vector<uint8_t>& mask, Cmp cmp);
};

class And : public IFilterExpression {
private:
    std::unique_ptr<IFilterExpression> left_;
    std::unique_ptr<IFilterExpression> right_;

public:
    And(std::unique_ptr<IFilterExpression>&& left, std::unique_ptr<IFilterExpression>&& right);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;
};

class Like : public IFilterExpression {
private:
    std::unique_ptr<IValueExpression> column_;
    std::string pattern_;

public:
    Like(std::unique_ptr<IValueExpression>&& column, const std::string& pattern);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;
};

}  // namespace columnar
