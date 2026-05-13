#include "Aggregation.h"

#include <ext/pb_ds/assoc_container.hpp>

#include "ExpressionsCore.h"
#include "glog/logging.h"
#include "ankerl/unordered_dense.h"

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

struct PhysTypeHash {
    template <typename T>
    size_t operator()(const T& x) const {
        return PhysTypeVariantHash{}(PhysTypeVariant{x});
    }
};

struct IAggregationState {
    virtual ~IAggregationState() = default;
    virtual void Update(const Column& col) = 0;
    virtual Column Finalize() = 0;
};

template <Type type>
class ITypeAggregationState : public IAggregationState {
public:
    using IAggregationState::Update;
    virtual void Update(const PhysicalType<type>&) = 0;
};

template <Type type>
class SumState : public ITypeAggregationState<type> {
    static constexpr Type kSumType = Type::Int128;

public:
    void Update(const Column& col) override {
        sum_ += std::get<PhysicalType<kSumType>>(Sum(col));
    }

    void Update(const PhysicalType<type>& val) override {
        if constexpr (type == Type::String || type == Type::MetaString || type == Type::Timestamp ||
                      type == Type::Date) {
            throw std::runtime_error("[SumState]: unsupported type for SUM");
        } else {
            sum_ += val;
        }
    }

    Column Finalize() override {
        return Column(ArrayType<kSumType>{sum_});
    }

private:
    PhysicalType<kSumType> sum_ = 0;
};

template <Type type>
class DistinctState : public ITypeAggregationState<type> {
public:
    void Update(const Column& col) override {
        const auto& data = std::get<ArrayType<type>>(col.GetData());
        for (size_t i = 0; i < data.size(); i++) {
            seen_.insert({data[i], false});
        }
    }

    void Update(const PhysicalType<type>& val) override {
        seen_.insert({val, false});
    }

    Column Finalize() override {
        return Column(ArrayType<Type::UInt64>{seen_.size()});
    }

private:
    __gnu_pbds::gp_hash_table<PhysicalType<type>, bool, PhysTypeHash> seen_;
};

template <Type type>
class CountState : public ITypeAggregationState<type> {
    void Update(const Column& col) override {
        count_ += col.Size();
    }

    void Update(const PhysicalType<type>&) override {
        count_++;
    }

    Column Finalize() override {
        return Column(ArrayType<Type::UInt64>{count_});
    }

private:
    PhysicalType<Type::UInt64> count_ = 0;
};

template <Type type>
class MinState : public ITypeAggregationState<type> {
public:
    void Update(const Column& col) override {
        auto batch_min = Min(col);
        if (!batch_min) {
            return;
        }
        auto real_batch_min = std::get<PhysicalType<type>>(*batch_min);
        if (!min_ || real_batch_min < *min_) {
            min_ = real_batch_min;
        }
    }

    void Update(const PhysicalType<type>& val) override {
        if (!min_ || val < *min_) {
            min_ = val;
        }
    }

    Column Finalize() override {
        if (!min_) {
            throw std::runtime_error("[MinState]: empty input");
        }
        return Column(ArrayType<type>{*min_});
    }

private:
    std::optional<PhysicalType<type>> min_;
};

template <Type type>
class MaxState : public ITypeAggregationState<type> {
public:
    void Update(const Column& col) override {
        auto batch_max = Max(col);
        if (!batch_max) {
            return;
        }
        auto real_batch_max = std::get<PhysicalType<type>>(*batch_max);
        if (!max_ || real_batch_max > *max_) {
            max_ = real_batch_max;
        }
    }

    void Update(const PhysicalType<type>& val) override {
        if (!max_ || val > *max_) {
            max_ = val;
        }
    }

    Column Finalize() override {
        if (!max_) {
            throw std::runtime_error("[MaxState]: empty input");
        }
        return Column(ArrayType<type>{*max_});
    }

private:
    std::optional<PhysicalType<type>> max_;
};

void MakeKey(const std::vector<Column>& key_cols, size_t row, std::string& key_buf) {
    key_buf.clear();

    for (const auto& kc : key_cols) {
        DispatchOnType(kc.GetType(), [&]<Type type>() {
            const auto val = std::get<PhysicalType<type>>(kc[row]);
            if constexpr (std::is_same_v<PhysicalType<type>, PhysicalType<Type::String>> ||
                          std::is_same_v<PhysicalType<type>, PhysicalType<Type::MetaString>>) {
                uint32_t len = static_cast<uint32_t>(val.size());
                key_buf.append(reinterpret_cast<const char*>(&len), sizeof(len));
                key_buf.append(val.data(), len);
            } else {
                key_buf.append(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        });
    }
}

std::vector<PhysTypeVariant> ExtractKey(const std::vector<Column>& key_cols, size_t row) {
    std::vector<PhysTypeVariant> key;
    key.reserve(key_cols.size());
    for (const auto& kc : key_cols) {
        key.push_back(kc[row]);
    }
    return key;
}

std::unique_ptr<IAggregationState> MakeState(AggregationType agg_type, Type col_type) {
    return DispatchOnType(col_type, [agg_type]<Type type>() -> std::unique_ptr<IAggregationState> {
        switch (agg_type) {
            case AggregationType::Sum:
                return std::make_unique<SumState<type>>();
            case AggregationType::Distinct:
                return std::make_unique<DistinctState<type>>();
            case AggregationType::Min:
                return std::make_unique<MinState<type>>();
            case AggregationType::Max:
                return std::make_unique<MaxState<type>>();
            case AggregationType::Count:
                return std::make_unique<CountState<type>>();
        }
        throw std::invalid_argument("[MakeState]: unknown aggregation type");
    });
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

    if (group_by_.empty()) {
        return GlobalNext();
    }

    const size_t n = aggregation_meta_.size();
    const size_t nk = group_by_.size();

    ankerl::unordered_dense::map<std::string, size_t> index_map;
    std::vector<std::vector<PhysTypeVariant>> group_keys;
    std::vector<std::vector<std::unique_ptr<IAggregationState>>> group_states;
    std::vector<Type> key_out_types;

    std::vector<std::vector<std::shared_ptr<char[]>>> owning_buffers(nk + n);

    std::string key_buf;
    key_buf.reserve(256);

    while (auto batch_ptr = next_operator_->Next()) {
        const size_t rows = (*batch_ptr)->RowCount();
        if (rows == 0) {
            continue;
        }

        std::vector<Column> key_cols;
        key_cols.reserve(nk);
        for (size_t i = 0; i < nk; i++) {
            key_cols.push_back(group_by_[i].expression->Calculate(*batch_ptr));
            const auto& buffer = key_cols.back().GetOwningBuffer();
            owning_buffers[i].insert(owning_buffers[i].end(), buffer.begin(), buffer.end());
        }

        if (key_out_types.empty()) {
            for (const auto& kc : key_cols) {
                key_out_types.push_back(kc.GetType());
            }
        }

        std::vector<Column> agg_cols;
        agg_cols.reserve(n);
        for (size_t i = 0; i < n; i++) {
            agg_cols.push_back(aggregation_meta_[i].expression->Calculate(*batch_ptr));
            const auto& buffer = agg_cols.back().GetOwningBuffer();
            owning_buffers[nk + i].insert(owning_buffers[nk + i].end(), buffer.begin(),
                                          buffer.end());
        }

        std::vector<size_t> row_group_idx(rows);
        for (size_t row = 0; row < rows; row++) {
            MakeKey(key_cols, row, key_buf);
            auto [it, inserted] = index_map.try_emplace(key_buf, group_states.size());
            if (inserted) {
                group_keys.push_back(ExtractKey(key_cols, row));
                auto& states = group_states.emplace_back();
                states.reserve(n);
                for (size_t i = 0; i < n; i++) {
                    states.push_back(MakeState(aggregation_meta_[i].type, agg_cols[i].GetType()));
                }
            }
            row_group_idx[row] = it->second;
        }

        for (size_t j = 0; j < n; j++) {
            DispatchOnType(agg_cols[j].GetType(), [&]<Type type>() {
                const auto& data = std::get<ArrayType<type>>(agg_cols[j].GetData());
                for (size_t row = 0; row < rows; row++) {
                    auto* state = static_cast<ITypeAggregationState<type>*>(
                        group_states[row_group_idx[row]][j].get());
                    state->Update(data[row]);
                }
            });
        }
    }

    if (group_states.empty()) {
        return std::nullopt;
    }

    const size_t n_groups = group_states.size();

    std::vector<std::vector<Column>> group_results;
    group_results.reserve(n_groups);
    for (auto& states : group_states) {
        auto& gr = group_results.emplace_back();
        gr.reserve(n);
        for (auto& state : states) {
            gr.push_back(state->Finalize());
        }
    }

    std::vector<Column> out_cols;
    out_cols.reserve(nk + n);

    for (size_t k = 0; k < nk; ++k) {
        out_cols.push_back(DispatchOnType(key_out_types[k], [&]<Type type>() {
            ArrayType<type> data;
            data.reserve(n_groups);
            for (const auto& key : group_keys) {
                data.push_back(std::get<PhysicalType<type>>(key[k]));
            }
            return Column(std::move(data), owning_buffers[k]);
        }));
    }

    for (size_t j = 0; j < n; ++j) {
        Type agg_type = group_results[0][j].GetType();
        out_cols.push_back(DispatchOnType(agg_type, [&]<Type type>() {
            ArrayType<type> data;
            data.reserve(n_groups);
            for (const auto& gr : group_results) {
                data.push_back(std::get<PhysicalType<type>>(gr[j][0]));
            }
            return Column(std::move(data), owning_buffers[nk + j]);
        }));
    }

    std::vector<Schema::ColumnData> schema_data;
    schema_data.reserve(nk + n);
    for (size_t k = 0; k < nk; ++k) {
        schema_data.emplace_back(group_by_[k].result_column_name, key_out_types[k]);
    }
    for (size_t j = 0; j < n; ++j) {
        schema_data.emplace_back(aggregation_meta_[j].result_column_name,
                                 group_results[0][j].GetType());
    }

    auto result = std::make_shared<Batch>(Schema(std::move(schema_data)));
    for (auto& col : out_cols) {
        result->AddColumn(std::move(col));
    }

    DLOG(INFO) << "[Aggregation]: Finished\n";
    return result;
}

std::optional<std::shared_ptr<Batch>> Aggregation::GlobalNext() {
    const size_t n = aggregation_meta_.size();
    std::vector<std::unique_ptr<IAggregationState>> states;
    std::vector<std::shared_ptr<char[]>> owning_buffers;

    while (auto batch = next_operator_->Next()) {
        std::vector<Column> cols;
        cols.reserve(n);
        owning_buffers.reserve(owning_buffers.size() + n);
        for (size_t i = 0; i < n; i++) {
            cols.push_back(aggregation_meta_[i].expression->Calculate(*batch));
            const auto& buffer = cols.back().GetOwningBuffer();
            owning_buffers.insert(owning_buffers.end(), buffer.begin(), buffer.end());
        }

        if (states.empty()) {
            states.reserve(n);
            for (size_t i = 0; i < n; i++) {
                states.push_back(MakeState(aggregation_meta_[i].type, cols[i].GetType()));
            }
        }

        for (size_t i = 0; i < n; i++) {
            states[i]->Update(cols[i]);
        }
    }

    if (states.empty()) {
        return std::nullopt;
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

}  // namespace operators
}  // namespace cngn
