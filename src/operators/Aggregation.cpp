#include "Aggregation.h"

#include "ExpressionsCore.h"

#include "glog/logging.h"

#include <ext/pb_ds/assoc_container.hpp>


namespace cngn {
namespace operators {

struct PhysTypeVariantHash {
    size_t operator()(const PhysTypeVariant& v) const {
        return std::visit(
        []<typename T>(const T& x) -> size_t {
          using V = std::decay_t<T>;
          if constexpr (std::is_same_v<V, PhysicalType<Type::Int128>>) {
            const auto u = static_cast<__uint128_t>(x);
            size_t h = std::hash<uint64_t>{}(static_cast<uint64_t>(u));
            h ^= std::hash<uint64_t>{}(static_cast<uint64_t>(u >> 64)) + 0x8e3779b97a4a7c15ULL + (h << 6) + (h >> 2);
            return h;
          } else if constexpr (std::is_integral_v<V>) {
            return std::hash<V>{}(x);
          } else if constexpr (std::is_same_v<V, PhysicalType<Type::Date>>) {
            return std::hash<uint32_t>{}(x.days);
          } else if constexpr (std::is_same_v<V, PhysicalType<Type::Timestamp>>) {
            return std::hash<uint64_t>{}(x.seconds);
          } else if constexpr (std::is_same_v<V, PhysicalType<Type::String>> || std::is_same_v<V, PhysicalType<Type::MetaString>>) {
            return std::hash<V>{}(x);
          } else {
            static_assert(false, "[PhysTypeVariantHash]: Unknown type");
          }
        },
        v);
    }
};

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
    DLOG(INFO) << "[Aggregation]: Next\n";
    if (finished_) {
        return std::nullopt;
    }

    const size_t aggregations_count = aggregation_meta_.size();

    DLOG(INFO) << "[Aggregation]: Aggregations count: " << aggregations_count << "\n";

    std::vector<__int128_t> sum(aggregations_count, 0);
    std::vector<__gnu_pbds::gp_hash_table<PhysTypeVariant, bool, PhysTypeVariantHash>> distinct(aggregations_count);

    std::vector<Schema::ColumnData> schema_data(aggregations_count);

    for (size_t i = 0; i < aggregations_count; i++) {
        const auto& [aggr_type, expression, res_name] = aggregation_meta_[i];
        Type res_type;

        switch (aggr_type) {
            case AggregationType::Sum:
                res_type = Type::Int128;
                break;
            case AggregationType::Distinct:
                res_type = Type::UInt64;
                break;
            default:
                throw std::invalid_argument("[Aggregation]: Unknown aggregation type");
        }

        schema_data[i] = {res_name, res_type};
    }

    auto result = std::make_shared<Batch>(Schema(std::move(schema_data)));

    std::vector<Column> columns_holder;
    columns_holder.reserve(aggregations_count);

    while (auto batch = next_operator_->Next()) {
        for (size_t i = 0; i < aggregations_count; i++) {
            const auto& [type, expression, res_name] = aggregation_meta_[i];
            switch (type) {
                case AggregationType::Sum: {
                    columns_holder.push_back(expression->Calculate(batch.value()));
                    sum[i] += std::get<__int128_t>(Sum(columns_holder.back()));
                    break;
                }
                case AggregationType::Distinct: {
                    columns_holder.push_back(expression->Calculate(batch.value()));
                    const auto& col = columns_holder.back();
                    for (size_t row_num = 0; row_num < col.Size(); row_num++) {
                        distinct[i].insert(std::make_pair(col[row_num], false));
                    }
                    break;
                }
                default:
                    throw std::invalid_argument("[Aggregation]: Unknown aggregation type");
            }
        }
    }

    for (size_t i = 0; i < aggregations_count; i++) {
        const auto& [type, expression, res_name] = aggregation_meta_[i];
        switch (type) {
            case AggregationType::Sum:
                result->AddColumn(Column(ArrayType<Type::Int64>{static_cast<int64_t>(sum[i])}));
                break;
            case AggregationType::Distinct:
                result->AddColumn(Column(ArrayType<Type::UInt64>{distinct[i].size()}));
                break;
            default:
                throw std::invalid_argument("[Aggregation]: Unknown aggregation type");
        }
    }

    finished_ = true;

    DLOG(INFO) << "[Aggregation]: Finished\n";

    return result;
}

}  // namespace operators
}  // namespace cngn