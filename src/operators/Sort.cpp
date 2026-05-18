#include "Sort.h"

#include <glog/logging.h>

#include <numeric>
#include <queue>

namespace cngn {
namespace operators {

Sort::Sort(std::unique_ptr<Operator> next_operator, std::vector<SortKey> sort_meta,
           bool is_high_order)
    : next_operator_(std::move(next_operator)),
      sort_meta_(std::move(sort_meta)),
      is_high_order_(is_high_order) {
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
    DLOG(INFO) << "[Sort]: Started\n";
    if (finished_) {
        return std::nullopt;
    }

    finished_ = true;

    std::vector<std::shared_ptr<Batch>> batches;

    while (auto batch = next_operator_->Next()) {
        if (batch.value()->RowCount() > 0) {
            batches.push_back(std::move(batch.value()));
        }
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

    DLOG(INFO) << "[Sort]: Finished\n";

    return ReorderRows(glued, indices);
}

std::shared_ptr<Batch> Sort::GlueBatches(const std::vector<std::shared_ptr<Batch>>& batches) {
    DLOG(INFO) << "[Sort::GlueBatches]: Started\n";
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

    DLOG(INFO) << "[Sort::GlueBatches]: Finished\n";

    return result;
}

std::shared_ptr<Batch> Sort::ReorderRows(const std::shared_ptr<Batch>& batch,
                                         const std::vector<size_t>& indices) {
    DLOG(INFO) << "[Sort::ReorderRows]: Started\n";
    auto result = std::make_shared<Batch>(batch->GetSchema());

    for (size_t col_idx = 0; col_idx < batch->ColumnCount(); ++col_idx) {
        Column col = (*batch)[col_idx];
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

    DLOG(INFO) << "[Sort::ReorderRows]: Finished\n";

    return result;
}

TopK::TopK(std::unique_ptr<Operator> next_operator, std::vector<SortKey> sort_meta, size_t k,
           bool is_high_order, size_t offset)
    : next_operator_(std::move(next_operator)),
      sort_meta_(std::move(sort_meta)),
      k_(k),
      offset_(offset),
      is_high_order_(is_high_order) {

    if (k_ == 0) {
        throw std::invalid_argument("[TopK]: k must be greater than 0");
    }
    if (sort_meta_.empty()) {
        throw std::invalid_argument("[TopK]: sort_meta is empty");
    }
    if (next_operator_ == nullptr) {
        throw std::invalid_argument("[TopK]: next_operator is nullptr");
    }
}

void TopK::Open() {
    next_operator_->Open();
}

void TopK::Close() {
    next_operator_->Close();
}

std::optional<std::shared_ptr<Batch>> TopK::Next() {
    DLOG(INFO) << "[TopK]: Started\n";
    if (finished_) {
        return std::nullopt;
    }
    finished_ = true;

    struct HeapRow {
        std::vector<PhysTypeVariant> keys;
        std::vector<PhysTypeVariant> cols;
        std::vector<std::shared_ptr<char[]>> row_buffers;
    };

    auto cmp = [&](const HeapRow& a, const HeapRow& b) {
        for (size_t k = 0; k < sort_meta_.size(); k++) {
            if (a.keys[k] == b.keys[k]) {
                continue;
            }
            return is_high_order_ ? a.keys[k] < b.keys[k] : a.keys[k] > b.keys[k];
        }
        return false;
    };

    std::priority_queue<HeapRow, std::vector<HeapRow>, decltype(cmp)> pq(cmp);

    std::optional<Schema> schema;

    while (auto batch_opt = next_operator_->Next()) {
        auto batch = batch_opt.value();

        if (batch->RowCount() == 0) {
            continue;
        }

        if (!schema) {
            schema = batch->GetSchema();
        }

        std::vector<Column> key_cols;
        key_cols.reserve(sort_meta_.size());
        for (const auto& sk : sort_meta_) {
            key_cols.push_back(sk.expression->Calculate(batch));
        }

        std::vector<std::shared_ptr<char[]>> bufs;
        bufs.reserve(batch->ColumnCount());
        for (size_t c = 0; c < batch->ColumnCount(); ++c) {
            const auto& buf = (*batch)[c].GetOwningBuffer();
            bufs.insert(bufs.end(), buf.begin(), buf.end());
        }

        const size_t rows = batch->RowCount();
        for (size_t row = 0; row < rows; ++row) {
            HeapRow hr;
            hr.keys.reserve(sort_meta_.size());

            for (const auto& kc : key_cols) {
                hr.keys.push_back(kc[row]);
            }

            if (pq.size() < k_ + offset_ || cmp(hr, pq.top())) {
                hr.cols.reserve(batch->ColumnCount());
                for (size_t c = 0; c < batch->ColumnCount(); ++c) {
                    hr.cols.push_back((*batch)[c][row]);
                }
                hr.row_buffers = bufs;
                pq.push(std::move(hr));
            }
            if (pq.size() > k_ + offset_) {
                pq.pop();
            }
        }
    }

    if (pq.size() <= offset_) {
        return std::nullopt;
    }

    std::vector<HeapRow> rows;
    rows.reserve(pq.size() - offset_);
    for (size_t i = 0; i < offset_; i++) {
        pq.pop();
    }
    while (!pq.empty()) {
        rows.push_back(pq.top());
        pq.pop();
    }
    std::reverse(rows.begin(), rows.end());

    const size_t n_cols = rows[0].cols.size();
    auto result = std::make_shared<Batch>(*schema);

    for (size_t c = 0; c < n_cols; ++c) {
        Type col_type = static_cast<Type>(rows[0].cols[c].index());
        DispatchOnType(col_type, [&]<Type t>() {
            std::vector<std::shared_ptr<char[]>> bufs;
            bufs.reserve(rows.size());
            ArrayType<t> col_data;
            col_data.reserve(rows.size());
            for (const auto& hr : rows) {
                col_data.push_back(std::get<PhysicalType<t>>(hr.cols[c]));
                bufs.insert(bufs.end(), hr.row_buffers.begin(), hr.row_buffers.end());
            }
            result->AddColumn(Column(std::move(col_data), std::move(bufs)));
        });
    }

    DLOG(INFO) << "[TopK]: Finished\n";

    return result;
}

}  // namespace operators
}  // namespace cngn