#include "Aggregation.h"

#include "ExpressionsCore.h"

namespace cngn {
namespace operators {

Aggregation::Aggregation(std::unique_ptr<Operator> next_operator,
                         std::vector<AggregationMeta> aggregation_meta)
    : next_operator_(std::move(next_operator)), aggregation_meta_(std::move(aggregation_meta)) {
    if (next_operator_ == nullptr) {
        throw std::invalid_argument("[Aggregation]: next_operator is nullptr");
    }
    if (aggregation_meta_.empty()) {
        throw std::invalid_argument("[Aggregation]: Aggregation meta is empty");
    }
}

void Aggregation::Open() {
    next_operator_->Open();
}

void Aggregation::Close() {
    next_operator_->Close();
}

std::optional<std::shared_ptr<Batch>> Aggregation::Next() {
    if (finished_) {
        return std::nullopt;
    }

    const size_t aggregations_count = aggregation_meta_.size();

    std::vector<__int128_t> sum(aggregations_count, 0);

    std::vector<Schema::ColumnData> schema_data(aggregations_count);

    for (size_t i = 0; i < aggregations_count; i++) {
        const auto& [aggr_type, expression, res_name] = aggregation_meta_[i];
        Type res_type;

        switch (aggr_type) {
            case AggregationType::Sum:
                res_type = Type::Int128;
                break;
            default:
                throw std::invalid_argument("[Aggregation]: Unknown aggregation type");
        }

        schema_data[i] = {res_name, res_type};
    }

    auto result = std::make_shared<Batch>(Schema(std::move(schema_data)));

    while (auto batch = next_operator_->Next()) {
        for (size_t i = 0; i < aggregations_count; i++) {
            const auto& [type, expression, res_name] = aggregation_meta_[i];
            switch (type) {
                case AggregationType::Sum:
                    Column col = expression->Calculate(batch.value());
                    sum[i] += Sum(col);
                    break;
                // default:
                    // throw std::invalid_argument("[Aggregation]: Unknown aggregation type");
            }
        }
    }

    for (size_t i = 0; i < aggregations_count; i++) {
        const auto& [type, expression, res_name] = aggregation_meta_[i];
        switch (type) {
            case AggregationType::Sum:
                result->AddColumn(Column(ArrayType<Type::Int128>{sum[i]}));
                break;
            default:
                throw std::invalid_argument("[Aggregation]: Unknown aggregation type");
        }
    }

    finished_ = true;

    return result;
}

}  // namespace operators
}  // namespace cngn