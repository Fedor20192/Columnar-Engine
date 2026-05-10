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

struct VectorHash {
    size_t operator()(const std::vector<PhysTypeVariant>& vec) const {
        size_t seed = vec.size();
        for (const auto& v : vec) {
            seed ^= PhysTypeVariantHash{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

struct IAggregationState {
    virtual ~IAggregationState() = default;
    virtual void Update(const Column& col) = 0;
    virtual Column Finalize() = 0;
};

class SumState : public IAggregationState {
    static const Type kSumType = Type::Int128;

public:
    void Update(const Column& col) override {
        sum_ += std::get<PhysicalType<kSumType>>(Sum(col));
    }

    Column Finalize() override {
        return Column(ArrayType<kSumType>{sum_});
    }

private:
    PhysicalType<kSumType> sum_ = 0;
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
    std::vector<Column> storage_;
};

class CountState : public IAggregationState {
    void Update(const Column& col) override {
        count_ += col.Size();
    }

    Column Finalize() override {
        return Column(ArrayType<Type::UInt64>{count_});
    }

private:
    PhysicalType<Type::UInt64> count_ = 0;
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

Column Slice(const Column& col, const std::vector<size_t>& indices) {
    if (col.GetType() == Type::String) {
        Column result(Type::MetaString, indices.size());
        const auto& arr = std::get<ArrayType<Type::String>>(col.GetData());
        for (size_t i : indices) {
            result.PushBack<Type::MetaString>(std::string(arr[i]));
        }
        return result;
    }
    Column result(col.GetType(), indices.size());
    DispatchOnType(col.GetType(), [&]<Type t>() {
        const auto& arr = std::get<ArrayType<t>>(col.GetData());
        for (size_t i : indices) {
            result.PushBack<t>(arr[i]);
        }
    });
    return result;
}

std::vector<PhysTypeVariant> MakeKey(const std::vector<Column>& key_cols, size_t row) {
    std::vector<PhysTypeVariant> key;
    key.reserve(key_cols.size());
    for (const auto& kc : key_cols) {
        PhysTypeVariant val = kc[row];
        if (std::holds_alternative<PhysicalType<Type::String>>(val)) {
            key.push_back(std::string(std::get<PhysicalType<Type::String>>(val)));
        } else {
            key.push_back(std::move(val));
        }
    }
    return key;
}

std::unique_ptr<IAggregationState> MakeState(AggregationType type) {
    switch (type) {
        case AggregationType::Sum:
            return std::make_unique<SumState>();
        case AggregationType::Distinct:
            return std::make_unique<DistinctState>();
        case AggregationType::Min:
            return std::make_unique<MinState>();
        case AggregationType::Max:
            return std::make_unique<MaxState>();
        case AggregationType::Count:
            return std::make_unique<CountState>();
    }
    throw std::invalid_argument("[MakeState]: unknown aggregation type");
}

}  // namespace

Aggregation::Aggregation(std::unique_ptr<Operator> next_operator,
                         std::vector<AggregationMeta> aggregation_meta,
                         std::vector<GroupByMeta> group_by)
    : next_operator_(std::move(next_operator)),
      aggregation_meta_(std::move(aggregation_meta)),
      group_by_(std::move(group_by)) {
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
    finished_ = true;

    const size_t n = aggregation_meta_.size();

    if (group_by_.empty()) {
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

        DLOG(INFO) << "[Aggregation]: Finished\n";
        return result;
    }

    const size_t nk = group_by_.size();
    using Key = std::vector<PhysTypeVariant>;
    std::unordered_map<Key, std::vector<std::unique_ptr<IAggregationState>>, VectorHash> groups;
    std::vector<Type> key_out_types;

    while (auto batch_ptr = next_operator_->Next()) {
        const size_t rows = (*batch_ptr)->RowCount();
        if (rows == 0) {
            continue;
        }

        std::vector<Column> key_cols;
        key_cols.reserve(nk);
        for (const auto& meta : group_by_) {
            key_cols.push_back(meta.expression->Calculate(*batch_ptr));
        }

        if (key_out_types.empty()) {
            for (const auto& kc : key_cols) {
                Type t = kc.GetType();
                key_out_types.push_back(t == Type::String ? Type::MetaString : t);
            }
        }

        std::vector<Column> agg_cols;
        agg_cols.reserve(n);
        for (const auto& meta : aggregation_meta_) {
            agg_cols.push_back(meta.expression->Calculate(*batch_ptr));
        }

        std::unordered_map<Key, std::vector<size_t>, VectorHash> group_rows;
        for (size_t row = 0; row < rows; ++row) {
            group_rows[MakeKey(key_cols, row)].push_back(row);
        }

        for (const auto& [key, indices] : group_rows) {
            auto [it, inserted] = groups.try_emplace(key);
            if (inserted) {
                it->second.reserve(n);
                for (const auto& meta : aggregation_meta_) {
                    it->second.push_back(MakeState(meta.type));
                }
            }
            for (size_t j = 0; j < n; ++j) {
                it->second[j]->Update(Slice(agg_cols[j], indices));
            }
        }
    }

    if (groups.empty()) {
        return std::nullopt;
    }

    struct GroupResult {
        Key key;
        std::vector<Column> agg_cols;
    };
    std::vector<GroupResult> results;
    results.reserve(groups.size());
    for (auto& [key, states] : groups) {
        GroupResult gr;
        gr.key = key;
        gr.agg_cols.reserve(n);
        for (auto& state : states) {
            gr.agg_cols.push_back(state->Finalize());
        }
        results.push_back(std::move(gr));
    }

    const size_t n_groups = results.size();

    std::vector<Column> out_cols;
    out_cols.reserve(nk + n);

    for (size_t k = 0; k < nk; ++k) {
        Column col(key_out_types[k], n_groups);
        for (const auto& gr : results) {
            DispatchOnType(key_out_types[k], [&]<Type t>() {
                col.PushBack<t>(std::get<PhysicalType<t>>(gr.key[k]));
            });
        }
        out_cols.push_back(std::move(col));
    }

    for (size_t j = 0; j < n; ++j) {
        Type agg_type = results[0].agg_cols[j].GetType();
        Column col(agg_type, n_groups);
        for (const auto& gr : results) {
            DispatchOnType(agg_type, [&]<Type t>() {
                col.PushBack<t>(std::get<PhysicalType<t>>(gr.agg_cols[j][0]));
            });
        }
        out_cols.push_back(std::move(col));
    }

    std::vector<Schema::ColumnData> schema_data;
    schema_data.reserve(nk + n);
    for (size_t k = 0; k < nk; ++k) {
        schema_data.emplace_back(group_by_[k].result_column_name, key_out_types[k]);
    }
    for (size_t j = 0; j < n; ++j) {
        schema_data.emplace_back(aggregation_meta_[j].result_column_name, results[0].agg_cols[j].GetType());
    }

    auto result = std::make_shared<Batch>(Schema(std::move(schema_data)));
    for (auto& col : out_cols) {
        result->AddColumn(std::move(col));
    }

    DLOG(INFO) << "[Aggregation]: Finished\n";
    return result;
}

}  // namespace operators
}  // namespace cngn
