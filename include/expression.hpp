#pragma once

#include <memory>
#include <re2/re2.h>
#include <variant>

#include "batch.hpp"
#include "column.hpp"

namespace columnar {

class IFilterExpression;

class IValueExpression {
public:
    virtual const Column* evaluate(const Batch& batch) = 0;
    virtual Type output_type() const = 0;
    virtual ~IValueExpression() = default;
};

class ColumnRef : public IValueExpression {
private:
    std::string name_;
    Column column_;
    std::unique_ptr<IValueExpression> expr_ = nullptr;

public:
    ColumnRef(const std::string& name, std::unique_ptr<IValueExpression>&& expr = nullptr);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;

    std::string name() const;
};

class Literal : public IValueExpression {
private:
    std::variant<int64_t, std::string> literal_;
    Column column_;

public:
    template <typename T>
    Literal(T literal) : literal_(literal) {
    }

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class Add : public IValueExpression {
private:
    std::unique_ptr<IValueExpression> left_;
    std::unique_ptr<IValueExpression> right_;
    Column column_;

public:
    Add(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class Sub : public IValueExpression {
private:
    std::unique_ptr<IValueExpression> left_;
    std::unique_ptr<IValueExpression> right_;
    std::string output_name_;
    Column column_;

public:
    Sub(std::unique_ptr<IValueExpression>&& left, std::unique_ptr<IValueExpression>&& right);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class Extract : public IValueExpression {
public:
    enum class ExtractSpec {
        second,
        minute,
        hour,
    };

private:
    std::unique_ptr<IValueExpression> target_;
    ExtractSpec spec_;
    Column column_;

public:
    Extract(std::unique_ptr<IValueExpression>&& target, ExtractSpec spec);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class StrLen : public IValueExpression {
private:
    std::unique_ptr<IValueExpression> target_;
    Column column_;

public:
    StrLen(std::unique_ptr<IValueExpression>&& target);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class DateTrunc : public IValueExpression {
public:
    enum class TruncSpec {
        second,
        minute,
        hour,
    };

private:
    std::unique_ptr<IValueExpression> target_;
    TruncSpec spec_;
    std::string output_name_;
    Column column_;

public:
    DateTrunc(std::unique_ptr<IValueExpression>&& target, TruncSpec spec);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class RegexpReplace : public IValueExpression {
private:
    std::unique_ptr<IValueExpression> target_;
    std::string replacement_;
    re2::RE2 pattern_;
    std::string output_name_;
    Column column_;

public:
    RegexpReplace(std::unique_ptr<IValueExpression>&& target, const std::string& pattern,
                  const std::string& replacement);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
};

class CaseWhen : public IValueExpression {
private:
    std::unique_ptr<IFilterExpression> condition_;
    std::unique_ptr<IValueExpression> then_;
    std::unique_ptr<IValueExpression> else_;
    Column column_;

public:
    CaseWhen(std::unique_ptr<IFilterExpression>&& cond, std::unique_ptr<IValueExpression>&& th,
             std::unique_ptr<IValueExpression>&& el);

    const Column* evaluate(const Batch& batch) override;

    Type output_type() const override;
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
            std::cerr << "Wrong type" << "\n  at " << __FILE__ << ":" << __LINE__ << "\n  in "
                      << __func__ << std::endl;
            std::abort();
            break;
        }
    }

    template <typename T, typename F>
    void compare(const Column& left, const Column& right, std::vector<uint8_t>& mask,
                 Cmp cmp) const {
        F val = right.get_value<F>(0);
        switch (cmp) {
        case Cmp::NE:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] == val) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::E:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] != val) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::GE:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] < val) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::LE:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] > val) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::G:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] <= val) {
                    mask[i] = 0;
                }
            }
            break;
        case Cmp::L:
            for (size_t i = 0; i < left.size(); ++i) {
                if (left.data_as<T>()[i] >= val) {
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
    re2::RE2 pattern_;

public:
    Like(std::unique_ptr<IValueExpression>&& column, const std::string& str);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;

    static std::string str_to_pattern(const std::string& str);
};

class NotLike : public IFilterExpression {
private:
    std::unique_ptr<IValueExpression> column_;
    re2::RE2 pattern_;

public:
    NotLike(std::unique_ptr<IValueExpression>&& column, const std::string& str);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;

    static std::string str_to_pattern(const std::string& str);
};

class In : public IFilterExpression {
private:
    std::unique_ptr<IValueExpression> column_;
    std::vector<int> set_;

public:
    In(std::unique_ptr<IValueExpression>&& column, const std::vector<int>& set);

    void evaluate(const Batch& batch, std::vector<uint8_t>& mask) override;
};

}  // namespace columnar
