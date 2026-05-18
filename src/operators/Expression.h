#pragma once

#include <regex>

#include "../kernel/Batch.h"

namespace cngn {
namespace operators {

enum class BinaryExpressionType {
    Neq,
    Eq,
    Gt,
    Geq,
    Add,
    Mul,
    Div,
    And,
};

struct Expression {
    virtual Column Calculate(std::shared_ptr<Batch> batch) const = 0;

    virtual ~Expression() = default;

protected:
    Expression() = default;
};

struct ConstantExpression : Expression {
    explicit ConstantExpression(PhysTypeVariant value) : value(std::move(value)) {
    }

    Column Calculate(std::shared_ptr<Batch>) const override;

    const PhysTypeVariant value;
};

struct SelectExpression : Expression {
    explicit SelectExpression(std::string column_name) : column_name(std::move(column_name)) {
    }

    Column Calculate(std::shared_ptr<Batch> batch) const override;

    const std::string column_name;
};

struct ExtractMinute : Expression {
    explicit ExtractMinute(std::shared_ptr<Expression> expression)
        : expression(std::move(expression)) {
    }

    Column Calculate(std::shared_ptr<Batch> batch) const override;

    std::shared_ptr<Expression> expression;
};

struct ContainsExpression : Expression {
    explicit ContainsExpression(std::shared_ptr<Expression> expression, std::string substr,
                                bool no = false)
        : expression(std::move(expression)), substr(std::move(substr)), no(no) {
    }

    Column Calculate(std::shared_ptr<Batch> batch) const override;

    std::shared_ptr<Expression> expression;
    const std::string substr;
    bool no;
};

struct StrLenExpression : Expression {
    explicit StrLenExpression(std::shared_ptr<Expression> expression)
        : expression(std::move(expression)) {
    }

    Column Calculate(std::shared_ptr<Batch> batch) const override;

    std::shared_ptr<Expression> expression;
};

struct RegexExpression : Expression {
    explicit RegexExpression(std::shared_ptr<Expression> expression, std::string pattern,
                             std::string rep)
        : expression(std::move(expression)), rep(std::move(rep)), reg(std::move(pattern)) {
    }

    Column Calculate(std::shared_ptr<Batch> batch) const override;

    std::shared_ptr<Expression> expression;
    const std::string rep;
    const std::regex reg;
};

struct BinaryExpression : Expression {
    explicit BinaryExpression(const BinaryExpressionType type, std::shared_ptr<Expression> left_son,
                              std::shared_ptr<Expression> right_son)
        : left(std::move(left_son)), right(std::move(right_son)), type(type) {
        if (!left || !right) {
            throw std::runtime_error("[Binary expression]: left or right are both");
        }
    }

    Column Calculate(std::shared_ptr<Batch> batch) const override;

    std::shared_ptr<Expression> left, right;
    const BinaryExpressionType type;
};
}  // namespace operators
}  // namespace cngn