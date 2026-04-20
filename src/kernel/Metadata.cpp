#include "Metadata.h"

#include "glog/logging.h"

namespace cngn {
Metadata::Metadata(const Schema& schema) : schema_(schema) {
}

Metadata::Metadata(Schema schema, std::vector<uint64_t> batch_offsets,
                   std::vector<uint64_t> rows_cnt, std::vector<uint64_t> columns_offsets)
    : schema_(std::move(schema)),
      batch_offsets_(std::move(batch_offsets)),
      rows_cnt_(std::move(rows_cnt)),
      columns_offsets_(std::move(columns_offsets)) {
}

const Schema& Metadata::GetSchema() const {
    return schema_;
}

const std::vector<uint64_t>& Metadata::GetBatchesOffsets() const {
    return batch_offsets_;
}

uint64_t Metadata::GetColumnOffset(uint64_t batch_index, uint64_t column_index) const {
    if (batch_index >= batch_offsets_.size()) {
        throw std::out_of_range("batch_index out of range: " + std::to_string(batch_index) +
                                ">=" + std::to_string(batch_offsets_.size()));
    }
    uint64_t columns_cnt = GetColumnsCnt();
    if (column_index >= columns_cnt) {
        throw std::out_of_range("column index out of range: " + std::to_string(column_index) +
                                ">=" + std::to_string(column_index));
    }
    return columns_offsets_[batch_index * columns_cnt + column_index];
}

uint64_t Metadata::GetColumnsCnt() const {
    return schema_.GetColumnsCount();
}

const std::vector<uint64_t>& Metadata::GetRowsCnt() const {
    return rows_cnt_;
}

void Metadata::AddBatch(size_t offset, size_t rows, std::vector<uint64_t>&& columns_offsets) {
    batch_offsets_.push_back(now_offset_);
    now_offset_ = offset;
    rows_cnt_.push_back(rows);
    columns_offsets_.reserve(columns_offsets.size() + columns_offsets_.size());
    columns_offsets_.insert(columns_offsets_.end(),
                            std::make_move_iterator(columns_offsets.begin()),
                            std::make_move_iterator(columns_offsets.end()));

    DLOG(INFO) << "Added batch info:"
                  "Offset: "
               << now_offset_ << '\n'
               << "Rows count: " << rows << '\n';
}

uint64_t Metadata::GetNowOffset() const {
    return now_offset_;
}

size_t Metadata::GetBatchCnt() const {
    return batch_offsets_.size();
}

void Metadata::SetNowOffset(uint64_t offset) {
    now_offset_ = offset;
}

std::vector<PhysTypeVariant> Metadata::Serialize() const {
    uint64_t old_offset = now_offset_;
    uint64_t batches_cnt = rows_cnt_.size();
    std::vector<PhysTypeVariant> result = schema_.Serialize();

    result.reserve(result.size() + batches_cnt * 2 + columns_offsets_.size() + 3);

    result.emplace_back(batches_cnt);
    result.emplace_back(columns_offsets_.size());

    for (size_t i = 0; i < rows_cnt_.size(); i++) {
        result.emplace_back(batch_offsets_[i]);
        result.emplace_back(rows_cnt_[i]);
    }

    for (size_t i = 0; i < columns_offsets_.size(); i++) {
        result.emplace_back(columns_offsets_[i]);
    }

    result.emplace_back(old_offset);

    return result;
}

}  // namespace cngn