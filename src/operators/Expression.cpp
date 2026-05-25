#include "Expression.h"

#include "ExpressionsCore.h"

namespace cngn {
namespace operators {
Column ConstantExpression::Calculate(std::shared_ptr<Batch> batch) const {
    auto rows_cnt = batch->RowCount();
    auto func = [rows_cnt]<typename T0>(T0 &&value) -> Column {
        using T = std::decay_t<T0>;
        return Column(std::vector<T>(rows_cnt, std::move(value)));
    };
    return std::visit(func, value);
}

Column BinaryExpression::Calculate(std::shared_ptr<Batch> batch) const {
    const Column left_res = left->Calculate(batch);
    const Column right_res = right->Calculate(batch);

    switch (type) {
        case BinaryExpressionType::Neq:
            return NotEqual(left_res, right_res);
        case BinaryExpressionType::Eq:
            return Equal(left_res, right_res);
        case BinaryExpressionType::Gt:
            return Gt(left_res, right_res);
        case BinaryExpressionType::Geq:
            return Geq(left_res, right_res);
        case BinaryExpressionType::Add:
            return Add(left_res, right_res);
        case BinaryExpressionType::Mul:
            return Mul(left_res, right_res);
        case BinaryExpressionType::Div:
            return Div(left_res, right_res);
        case BinaryExpressionType::And:
            return And(left_res, right_res);
        case BinaryExpressionType::Or:
            return Or(left_res, right_res);
        default:
            throw std::logic_error("[BinaryExpression:Calculate]: Unknown expression type");
    }
}

Column SelectExpression::Calculate(std::shared_ptr<Batch> batch) const {
    auto column = batch->GetColumnByName(column_name);
    return column;
}

Column ExtractMinute::Calculate(std::shared_ptr<Batch> batch) const {
    const Column res = expression->Calculate(batch);
    return ExtractMinuteFromCol(res);
}

Column ContainsExpression::Calculate(std::shared_ptr<Batch> batch) const {
    const Column res = expression->Calculate(batch);
    return Contains(std::move(res), substr, no);
}

Column StrLenExpression::Calculate(std::shared_ptr<Batch> batch) const {
    const Column res = expression->Calculate(batch);
    return StrLen(std::move(res));
}

Column RegexExpression::Calculate(std::shared_ptr<Batch> batch) const {
    const Column res = expression->Calculate(batch);
    return Regex(std::move(res), reg);
}

Column CaseExpression::Calculate(std::shared_ptr<Batch> batch) const {
    const Column pred_res = pred->Calculate(batch);
    const Column true_res = true_expr->Calculate(batch);
    const Column false_res = false_expr->Calculate(batch);

    return Case(std::move(pred_res), std::move(true_res), std::move(false_res));
}
}  // namespace operators
}  // namespace cngn