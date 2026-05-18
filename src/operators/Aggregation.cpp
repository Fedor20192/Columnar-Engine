#include "Aggregation.h"

#include <ext/pb_ds/assoc_container.hpp>

#include "ExpressionsCore.h"
#include "StringArena.h"
#include "ankerl/unordered_dense.h"
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
            Update(data[i]);
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

template <>
class DistinctState<Type::String> : public ITypeAggregationState<Type::String> {
public:
    void Update(const Column& col) override {
        const auto& data = std::get<ArrayType<Type::String>>(col.GetData());
        for (size_t i = 0; i < data.size(); i++) {
            Update(data[i]);
        }
    }

    void Update(const PhysicalType<Type::String>& val) override {
        seen_.insert({PhysicalType<Type::MetaString>(val)});
    }

    Column Finalize() override {
        return Column(ArrayType<Type::UInt64>{seen_.size()});
    }

private:
    __gnu_pbds::gp_hash_table<PhysicalType<Type::MetaString>, __gnu_pbds::null_type, PhysTypeHash>
        seen_;
};

template <Type type>
class CountState : public ITypeAggregationState<type> {
public:
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

template <Type type, typename cmp>
class MinState : public ITypeAggregationState<type> {
public:
    void Update(const Column& col) override {
        const auto data = std::get<ArrayType<type>>(col.GetData());
        for (size_t i = 0; i < data.size(); i++) {
            Update(data[i]);
        }
    }

    void Update(const PhysicalType<type>& val) override {
        if (!min_ || cmp{}(val, *min_)) {
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

template <typename cmp>
class MinState<Type::String, cmp> : public ITypeAggregationState<Type::String> {
public:
    void Update(const Column& col) override {
        const auto data = std::get<ArrayType<Type::String>>(col.GetData());
        for (size_t i = 0; i < data.size(); i++) {
            Update(data[i]);
        }
    }

    void Update(const PhysicalType<Type::String>& val) override {
        if (!min_ || cmp{}(val, *min_)) {
            min_ = PhysicalType<Type::MetaString>(val);
        }
    }

    Column Finalize() override {
        if (!min_) {
            throw std::runtime_error("[MinState]: empty input");
        }
        return Column(ArrayType<Type::MetaString>{*min_});
    }

private:
    std::optional<PhysicalType<Type::MetaString>> min_;
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
                if (len > 0) {
                    key_buf.append(val.data(), len);
                }
            } else {
                key_buf.append(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        });
    }
}

void ExtractKey(const std::vector<Column>& key_cols, size_t row,
                std::vector<PhysTypeVariant>& group_keys) {
    for (const auto& kc : key_cols) {
        group_keys.push_back(kc[row]);
    }
}

void MakeState(AggregationType agg_type, Type col_type,
               std::vector<std::unique_ptr<IAggregationState>>& group_states) {
    return DispatchOnType(col_type, [agg_type, &group_states]<Type type>() -> void {
        switch (agg_type) {
            case AggregationType::Sum:
                group_states.push_back(std::make_unique<SumState<type>>());
                break;
            case AggregationType::Distinct:
                group_states.push_back(std::make_unique<DistinctState<type>>());
                break;
            case AggregationType::Min:
                group_states.push_back(
                    std::make_unique<MinState<type, std::less<PhysicalType<type>>>>());
                break;
            case AggregationType::Max:
                group_states.push_back(
                    std::make_unique<MinState<type, std::greater<PhysicalType<type>>>>());
                break;
            case AggregationType::Count:
                group_states.push_back(std::make_unique<CountState<type>>());
                break;
            default:
                throw std::invalid_argument("[MakeState]: unknown aggregation type");
        }
    });
}

void UpdateStates(const std::vector<Column>& agg_cols, const std::vector<size_t>& row_group_idx,
                  const std::vector<std::unique_ptr<IAggregationState>>& group_states, size_t rows,
                  size_t n) {
    for (size_t j = 0; j < n; j++) {
        DispatchOnType(agg_cols[j].GetType(), [&]<Type type>() {
            const auto& data = std::get<ArrayType<type>>(agg_cols[j].GetData());
            for (size_t row = 0; row < rows; row++) {
                size_t state_idx = row_group_idx[row] * n + j;
                auto* state =
                    static_cast<ITypeAggregationState<type>*>(group_states[state_idx].get());
                state->Update(data[row]);
            }
        });
    }
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

std::vector<Column> Aggregation::PrepareKeyCols(size_t nk, std::vector<Type>& key_out_types,
                                                const std::shared_ptr<Batch>& batch_ptr) const {
    std::vector<Column> key_cols;
    key_cols.reserve(nk);
    for (size_t i = 0; i < nk; i++) {
        key_cols.push_back(group_by_[i].expression->Calculate(batch_ptr));
    }

    if (key_out_types.empty()) {
        for (const auto& kc : key_cols) {
            key_out_types.push_back(kc.GetType());
        }
    }
    return key_cols;
}

std::vector<Column> Aggregation::PrepareValueCols(size_t nv,
                                                  const std::shared_ptr<Batch>& batch_ptr) const {
    std::vector<Column> agg_cols;
    agg_cols.reserve(nv);
    for (size_t i = 0; i < nv; i++) {
        agg_cols.push_back(aggregation_meta_[i].expression->Calculate(batch_ptr));
    }
    return agg_cols;
}

std::vector<Schema::ColumnData> Aggregation::GetSchemaData(
    const std::vector<Column>& out_cols, const std::vector<Type>& key_out_types) const {
    const size_t nk = group_by_.size(), n = aggregation_meta_.size();
    std::vector<Schema::ColumnData> schema_data;
    schema_data.reserve(nk + n);
    for (size_t k = 0; k < nk; ++k) {
        schema_data.emplace_back(group_by_[k].result_column_name, key_out_types[k]);
    }
    for (size_t j = 0; j < n; ++j) {
        schema_data.emplace_back(aggregation_meta_[j].result_column_name,
                                 out_cols[nk + j].GetType());
    }
    return schema_data;
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
    std::vector<PhysTypeVariant> group_keys;
    std::vector<std::unique_ptr<IAggregationState>> group_states;
    std::vector<Type> key_out_types;

    StringArena key_arena;

    std::string key_buf;
    key_buf.reserve(256);

    while (auto batch_ptr = next_operator_->Next()) {
        const size_t rows = (*batch_ptr)->RowCount();
        if (rows == 0) {
            continue;
        }

        auto key_cols = PrepareKeyCols(nk, key_out_types, *batch_ptr);
        auto agg_cols = PrepareValueCols(n, *batch_ptr);

        std::vector<size_t> row_group_idx(rows);
        for (size_t row = 0; row < rows; row++) {
            MakeKey(key_cols, row, key_buf);
            auto [it, inserted] = index_map.try_emplace(key_buf, index_map.size());
            if (inserted) {
                size_t group_idx = group_keys.size();
                ExtractKey(key_cols, row, group_keys);
                for (size_t i = 0; i < nk; i++) {
                    if (key_out_types[i] == Type::String) {
                        auto& slot = group_keys[group_idx + i];
                        slot = PhysicalType<Type::String>(
                            key_arena.Copy(std::get<PhysicalType<Type::String>>(slot)));
                    }
                }
                for (size_t i = 0; i < n; i++) {
                    MakeState(aggregation_meta_[i].type, agg_cols[i].GetType(), group_states);
                }
            }
            row_group_idx[row] = it->second;
        }

        UpdateStates(agg_cols, row_group_idx, group_states, rows, n);
    }

    if (group_states.empty()) {
        return std::nullopt;
    }

    const size_t n_groups = index_map.size();

    std::vector<Column> out_cols;
    out_cols.reserve(nk + n);

    auto key_buffers = key_arena.ReleaseBlocks();

    for (size_t k = 0; k < nk; ++k) {
        out_cols.push_back(DispatchOnType(key_out_types[k], [&]<Type type>() {
            ArrayType<type> data;
            data.reserve(n_groups);
            for (size_t i = k; i < group_keys.size(); i += nk) {
                const auto& key = group_keys[i];
                data.push_back(std::get<PhysicalType<type>>(key));
            }
            if constexpr (type == Type::String) {
                return Column(std::move(data), key_buffers);
            } else {
                return Column(std::move(data));
            }
        }));
    }

    for (size_t j = 0; j < n; ++j) {
        Column first_res = group_states[j]->Finalize();
        Type agg_type = first_res.GetType();

        out_cols.push_back(DispatchOnType(agg_type, [&]<Type type>() {
            ArrayType<type> data;
            data.reserve(n_groups);

            data.push_back(std::get<PhysicalType<type>>(first_res[0]));

            for (size_t g = 1; g < n_groups; ++g) {
                Column res = group_states[g * n + j]->Finalize();
                data.push_back(std::get<PhysicalType<type>>(res[0]));
            }
            return Column(std::move(data));
        }));
    }

    auto result = std::make_shared<Batch>(Schema(GetSchemaData(out_cols, key_out_types)));
    for (auto& col : out_cols) {
        result->AddColumn(std::move(col));
    }

    DLOG(INFO) << "[Aggregation]: Finished\n";
    return result;
}

std::optional<std::shared_ptr<Batch>> Aggregation::GlobalNext() {
    const size_t n = aggregation_meta_.size();
    std::vector<std::unique_ptr<IAggregationState>> states;

    while (auto batch = next_operator_->Next()) {
        auto cols = PrepareValueCols(n, batch.value());

        if (states.empty()) {
            states.reserve(n);
            for (size_t i = 0; i < n; i++) {
                MakeState(aggregation_meta_[i].type, cols[i].GetType(), states);
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

    auto result = std::make_shared<Batch>(Schema(GetSchemaData(columns)));
    for (auto& col : columns) {
        result->AddColumn(std::move(col));
    }

    DLOG(INFO) << "[Aggregation]: Finished\n";
    return result;
}

}  // namespace operators
}  // namespace cngn
