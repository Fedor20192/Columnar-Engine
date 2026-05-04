#pragma once

#include "Expression.h"
#include "Operator.h"

namespace cngn {
namespace operators {

enum class AggregationType {
    Sum,
};

struct AggregationMeta {
    AggregationType type;
    std::shared_ptr<Expression> expression;
    std::string result_column_name;
};

class Aggregation : public Operator {
public:
    explicit Aggregation(std::unique_ptr<Operator> next_operator,
                         std::vector<AggregationMeta> aggregation_meta);

    void Open() override;

    std::optional<std::shared_ptr<Batch>> Next() override;

    void Close() override;

private:
    std::unique_ptr<Operator> next_operator_;
    std::vector<AggregationMeta> aggregation_meta_;
    bool finished_{false};
};
}  // namespace operators
}  // namespace cngn