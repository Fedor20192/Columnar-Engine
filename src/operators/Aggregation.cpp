#include "Aggregation.h"

#include <ext/pb_ds/assoc_container.hpp>

#include "ExpressionsCore.h"
#include "glog/logging.h"

namespace cngn {
namespace operators {

namespace {

struct PhysTypeVariantHash {
    size_t operator()(const PhysTypeVariant& v) const {
        return std::visit(
            []<typename T>(const T& x) -> size_t {
                using V = std::decay_t<T>;
                if constexpr (std::is_same_v<V, PhysicalType<Type::Int128>>) {
                    const auto u = static_cast<__uint128_t>(x);
                    size_t h = std::hash<uint64_t>{}(static_cast<uint64_t>(u));
                    h ^= std::hash<uint64_t>{}(static_cast<uint64_t>(u >> 64)) +
                         0x8e3779b97a4a7c15ULL + (h << 6) + (h >> 2);
                    return h;
                } else if constexpr (std::is_integral_v<V>) {
                    return std::hash<V>{}(x);
                } else if constexpr (std::is_same_v<V, PhysicalType<Type::Date>>) {
                    return std::hash<uint32_t>{}(x.days);
                } else if constexpr (std::is_same_v<V, PhysicalType<Type::Timestamp>>) {
                    return std::hash<uint64_t>{}(x.seconds);
                } else if constexpr (std::is_same_v<V, PhysicalType<Type::String>> ||
                                     std::is_same_v<V, PhysicalType<Type::MetaString>>) {
                    return std::hash<V>{}(x);
                } else {
                    static_assert(false, "[PhysTypeVariantHash]: Unknown type");
                }
            },
            v);
    }
};

struct IAggregationState {
    virtual ~IAggregationState() = default;
    virtual void Update(const Column& col) = 0;
    virtual Column Finalize() = 0;
};

class SumState : public IAggregationState {
public:
    void Update(const Column& col) override {
        sum_ += std::get<__int128_t>(Sum(col));
    }

    Column Finalize() override {
        return Column(ArrayType<Type::Int128>{sum_});
    }

private:
    __int128_t sum_ = 0;
};

class DistinctState : public IAggregationState {
public:
    void Update(const Column& col) override {
        storage_.push_back(col);
        for (size_t i = 0; i < col.Size(); i++) {
            seen_.insert({col[i], false});
        }
    }

    Column Finalize() override {
        return Column(ArrayType<Type::UInt64>{seen_.size()});
    }

private:
    __gnu_pbds::gp_hash_table<PhysTypeVariant, bool, PhysTypeVariantHash> seen_;
    std::vector<Column> storage_;  // keeps string_view backing buffers alive
};

class MinState : public IAggregationState {
public:
    void Update(const Column& col) override {
        auto batch_min = Min(col);
        if (!batch_min) {
            return;
        }
        if (!min_) {
            min_ = batch_min;
            return;
        }
        std::visit(
            [this]<typename T>(const T& new_val) {
                if (new_val < std::get<T>(*min_)) {
                    *min_ = new_val;
                }
            },
            *batch_min);
    }

    Column Finalize() override {
        if (!min_) {
            throw std::runtime_error("[MinState]: empty input");
        }
        return std::visit([]<typename T>(T v) -> Column { return Column(std::vector<T>{v}); },
                          *min_);
    }

private:
    std::optional<PhysTypeVariant> min_;
};

class MaxState : public IAggregationState {
public:
    void Update(const Column& col) override {
        auto batch_max = Max(col);
        if (!batch_max) {
            return;
        }
        if (!max_) {
            max_ = batch_max;
            return;
        }
        std::visit(
            [this]<typename T>(const T& new_val) {
                if (new_val > std::get<T>(*max_)) {
                    *max_ = new_val;
                }
            },
            *batch_max);
    }

    Column Finalize() override {
        if (!max_) {
            throw std::runtime_error("[MaxState]: empty input");
        }
        return std::visit([]<typename T>(T v) -> Column { return Column(std::vector<T>{v}); },
                          *max_);
    }

private:
    std::optional<PhysTypeVariant> max_;
};

static std::unique_ptr<IAggregationState> MakeState(AggregationType type) {
    switch (type) {
        case AggregationType::Sum:
            return std::make_unique<SumState>();
        case AggregationType::Distinct:
            return std::make_unique<DistinctState>();
        case AggregationType::Min:
            return std::make_unique<MinState>();
        case AggregationType::Max:
            return std::make_unique<MaxState>();
    }
    throw std::invalid_argument("[MakeState]: unknown aggregation type");
}

}  // namespace

Aggregation::Aggregation(std::unique_ptr<Operator> next_operator,
                         std::vector<AggregationMeta> aggregation_meta)
    : next_operator_(std::move(next_operator)), aggregation_meta_(std::move(aggregation_meta)) {
    if (next_operator_ == nullptr) {
        throw std::invalid_argument("[Aggregation]: next_operator is nullptr");
    }
    if (aggregation_meta_.empty()) {
        throw std::invalid_argument("[Aggregation]: aggregation meta is empty");
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

    const size_t n = aggregation_meta_.size();

    std::vector<std::unique_ptr<IAggregationState>> states;
    states.reserve(n);
    for (const auto& meta : aggregation_meta_) {
        states.push_back(MakeState(meta.type));
    }

    while (auto batch = next_operator_->Next()) {
        for (size_t i = 0; i < n; i++) {
            states[i]->Update(aggregation_meta_[i].expression->Calculate(*batch));
        }
    }

    std::vector<Column> columns;
    columns.reserve(n);
    for (auto& state : states) {
        columns.push_back(state->Finalize());
    }

    std::vector<Schema::ColumnData> schema_data;
    schema_data.reserve(n);
    for (size_t i = 0; i < n; i++) {
        schema_data.push_back({aggregation_meta_[i].result_column_name, columns[i].GetType()});
    }

    auto result = std::make_shared<Batch>(Schema(std::move(schema_data)));
    for (auto& col : columns) {
        result->AddColumn(std::move(col));
    }

    finished_ = true;

    DLOG(INFO) << "[Aggregation]: Finished\n";

    return result;
}

}  // namespace operators
}  // namespace cngn
