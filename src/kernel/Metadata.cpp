#include "Metadata.h"

#include "glog/logging.h"

namespace cngn {
Metadata::Metadata(const Schema& schema) : schema_(schema) {
}

Metadata::Metadata(Schema schema, std::vector<uint64_t> batch_offsets,
                   std::vector<uint64_t> rows_cnt)
    : schema_(std::move(schema)),
      batch_offsets_(std::move(batch_offsets)),
      rows_cnt_(std::move(rows_cnt)) {
}

const Schema& Metadata::GetSchema() const {
    return schema_;
}

const std::vector<uint64_t>& Metadata::GetOffsets() const {
    return batch_offsets_;
}

uint64_t Metadata::GetColumnsCnt() const {
    return schema_.GetColumnsCount();
}

const std::vector<uint64_t>& Metadata::GetRowsCnt() const {
    return rows_cnt_;
}

void Metadata::AddBatch(size_t offset, size_t columns, size_t rows) {
    if (GetColumnsCnt() != columns) {
        DLOG(ERROR) << "Bad columns count: " << columns << " != " << GetColumnsCnt() << '\n';
        throw std::runtime_error("Bad columns count");
    }
    batch_offsets_.push_back(now_offset_);
    now_offset_ = offset;
    rows_cnt_.push_back(rows);

    DLOG(INFO) << "Added batch info:"
                  "Offset: "
               << now_offset_ << '\n'
               << "Columns count: " << columns << '\n'
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

    result.reserve(result.size() + batches_cnt * 2 + 3);

    result.emplace_back(batches_cnt);

    for (size_t i = 0; i < rows_cnt_.size(); i++) {
        result.emplace_back(batch_offsets_[i]);
        result.emplace_back(rows_cnt_[i]);
    }

    result.emplace_back(old_offset);

    return result;
}

}  // namespace cngn