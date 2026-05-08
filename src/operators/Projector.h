#pragma once
#include "Expression.h"
#include "Operator.h"

namespace cngn {
namespace operators {

struct ProjectionMeta {
    std::shared_ptr<Expression> expression;
    std::string result_column_name;
};

class Projector : public Operator {
public:
    explicit Projector(std::unique_ptr<Operator> next_operator,
                       std::vector<ProjectionMeta> projections)
        : projections_(std::move(projections)), next_operator_(std::move(next_operator)) {
        if (!next_operator_) {
            throw std::invalid_argument("[Projector]: Next operator is null");
        }

        if (projections_.empty()) {
            throw std::invalid_argument("[Projector]: No projections");
        }

        for (size_t i = 0; i < projections_.size(); ++i) {
            if (!projections_[i].expression) {
                throw std::invalid_argument("[Projector]: Projection expression is null");
            }
        }
    }

    void Open() override {
        next_operator_->Open();
    }

    void Close() override {
        next_operator_->Close();
    }

    std::optional<std::shared_ptr<Batch>> Next() override {
        auto batch = next_operator_->Next().value_or(nullptr);

        if (!batch) {
            return std::nullopt;
        }

        std::vector<Column> projected_columns;
        projected_columns.reserve(projections_.size());

        std::vector<Schema::ColumnData> column_data;
        column_data.reserve(projections_.size());

        for (const auto& [expression, name] : projections_) {
            projected_columns.emplace_back(expression->Calculate(batch));
            column_data.emplace_back(name, projected_columns.back().GetType());
        }

        return std::make_shared<Batch>(std::move(projected_columns),
                                       Schema(std::move(column_data)));
    }

private:
    std::vector<ProjectionMeta> projections_;
    std::unique_ptr<Operator> next_operator_;
};

}  // namespace operators
}  // namespace cngn