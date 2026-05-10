#include "Sort.h"

#include <numeric>

namespace cngn {
namespace operators {

Sort::Sort(std::unique_ptr<Operator> next_operator, std::vector<SortKey> sort_meta, bool is_high_order)
    : next_operator_(std::move(next_operator)), sort_meta_(std::move(sort_meta)), is_high_order_(is_high_order) {
    if (next_operator_ == nullptr) {
        throw std::invalid_argument("[Sort]: next_operator is nullptr");
    }
    if (sort_meta_.empty()) {
        throw std::invalid_argument("[Sort]: sort meta is empty");
    }
}

void Sort::Open() {
    next_operator_->Open();
}

void Sort::Close() {
    next_operator_->Close();
}

std::optional<std::shared_ptr<Batch>> Sort::Next() {
    if (finished_) {
        return std::nullopt;
    }

    finished_ = true;

    std::vector<std::shared_ptr<Batch>> batches;

    while (auto batch = next_operator_->Next()) {
        batches.push_back(std::move(batch.value()));
    }

    if (batches.empty()) {
        return std::nullopt;
    }

    auto glued = GlueBatches(std::move(batches));

    const size_t rows_cnt = glued->RowCount();

    if (rows_cnt == 0) {
        return glued;
    }

    std::vector<Column> keys_columns;
    keys_columns.reserve(sort_meta_.size());
    for (size_t k = 0; k < sort_meta_.size(); k++) {
        keys_columns.emplace_back(sort_meta_[k].expression->Calculate(glued));
    }

    std::vector<size_t> indices(rows_cnt);
    std::iota(indices.begin(), indices.end(), 0);

    auto cmp = [this, &keys_columns](size_t a, size_t b) {
        for (size_t k = 0; k < keys_columns.size(); k++) {
            const auto& elem_a = keys_columns[k][a];
            const auto& elem_b = keys_columns[k][b];

            if (elem_a == elem_b) {
                continue;
            }

            return is_high_order_ ? (elem_a < elem_b) : (elem_b < elem_a);
        }
        return false;
    };
    std::sort(indices.begin(), indices.end(), std::move(cmp));

    return ReorderRows(glued, indices);
}

std::shared_ptr<Batch> Sort::GlueBatches(const std::vector<std::shared_ptr<Batch>> &batches) {
    if (batches.empty()) {
        throw std::invalid_argument("[Sort]: no batches to glue");
    }

    const auto& schema = batches[0]->GetSchema();
    const size_t cols_cnt = batches[0]->ColumnCount();

    size_t rows_cnt = 0;
    for (size_t i = 0; i < batches.size(); i++) {
        rows_cnt += batches[i]->RowCount();
    }

    auto result = std::make_shared<Batch>(schema);

    auto add_col = [rows_cnt, &batches]<Type type>(size_t col_idx) {
        ArrayType<type> column_data;
        column_data.reserve(rows_cnt);

        std::vector<std::shared_ptr<char[]>> buffers;

        for (size_t batch_num = 0; batch_num < batches.size(); batch_num++) {
            const auto& col = (*batches[batch_num])[col_idx];
            const auto& col_data = std::get<ArrayType<type>>(col.GetData());
            column_data.insert(column_data.end(), col_data.begin(), col_data.end());
            
            const auto& other_buffers = col.GetOwningBuffer();
            buffers.insert(buffers.end(), other_buffers.begin(), other_buffers.end());
        }

        return Column(std::move(column_data), std::move(buffers));
    };
    
    for (size_t col_idx = 0; col_idx < cols_cnt; col_idx++) {
        result->AddColumn(DispatchOnType((*batches[0])[col_idx].GetType(), add_col, col_idx));
    }

    return result;
}

std::shared_ptr<Batch> Sort::ReorderRows(const std::shared_ptr<Batch>& batch,
                                          const std::vector<size_t>& indices) {
    auto result = std::make_shared<Batch>(batch->GetSchema());

    for (size_t col_idx = 0; col_idx < batch->ColumnCount(); ++col_idx) {
        const Column& col = (*batch)[col_idx];
        result->AddColumn(DispatchOnType(col.GetType(), [&]<Type t>() {
            const auto& arr = std::get<ArrayType<t>>(col.GetData());
            ArrayType<t> data;
            data.reserve(indices.size());
            for (size_t i : indices) {
                data.push_back(arr[i]);
            }
            return Column(std::move(data), col.GetOwningBuffer());
        }));
    }

    return result;
}

}  // namespace operators
}  // namespace cngn