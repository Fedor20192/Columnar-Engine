#pragma once

#include "Expression.h"
#include "Operator.h"

namespace cngn {
namespace operators {

enum class AggregationType {
    Sum,
    Distinct,
    Count,
    Min,
    Max,
};

struct AggregationMeta {
    AggregationType type;
    std::shared_ptr<Expression> expression;
    std::string result_column_name;
};

struct GroupByMeta {
    std::shared_ptr<Expression> expression;
    std::string result_column_name;
};

class Aggregation : public Operator {
public:
    explicit Aggregation(std::unique_ptr<Operator> next_operator,
                         std::vector<AggregationMeta> aggregation_meta,
                         std::vector<GroupByMeta> group_by = {});

    void Open() override;

    std::optional<std::shared_ptr<Batch>> Next() override;

    void Close() override;

private:
    std::optional<std::shared_ptr<Batch>> GlobalNext() const;
    std::vector<Column> PrepareKeyCols(size_t nk, std::vector<Type> &key_out_types,
                                       const std::shared_ptr<Batch> &batch) const;
    std::vector<Column> PrepareValueCols(size_t nv, const std::shared_ptr<Batch> &batch) const;
    std::vector<Schema::ColumnData> GetSchemaData(
        const std::vector<Column> &out_cols, const std::vector<Type> &key_out_types = {}) const;

    std::unique_ptr<Operator> next_operator_;
    std::vector<AggregationMeta> aggregation_meta_;
    std::vector<GroupByMeta> group_by_;
    bool finished_{false};
};
}  // namespace operators
}  // namespace cngn