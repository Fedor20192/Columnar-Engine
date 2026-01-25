#include "Metadata.h"

#include "glog/logging.h"

namespace cngn {
Metadata::Metadata(const Schema& schema) : schema_(schema) {
}

Metadata::Metadata(Schema schema, std::vector<int64_t> batch_offsets,
                   std::vector<int64_t> columns_cnt, std::vector<int64_t> rows_cnt)
    : schema_(std::move(schema)),
      batch_offsets_(std::move(batch_offsets)),
      columns_cnt_(std::move(columns_cnt)),
      rows_cnt_(std::move(rows_cnt)) {
}

const Schema& Metadata::GetSchema() const {
    return schema_;
}

const std::vector<int64_t>& Metadata::GetOffsets() const {
    return batch_offsets_;
}

const std::vector<int64_t>& Metadata::GetColumnsCnt() const {
    return columns_cnt_;
}

const std::vector<int64_t>& Metadata::GetRowsCnt() const {
    return rows_cnt_;
}

void Metadata::AddBatch(size_t offset, size_t columns, size_t rows) {
    batch_offsets_.push_back(now_offset_);
    now_offset_ = offset;
    columns_cnt_.push_back(columns);
    rows_cnt_.push_back(rows);

    DLOG(INFO) << "Added batch info:"
                  "Offset: "
               << now_offset_ << '\n'
               << "Columns count: " << columns << '\n'
               << "Rows count: " << rows << std::endl;
}

int64_t Metadata::GetNowOffset() const {
    return now_offset_;
}

size_t Metadata::GetBatchCnt() const {
    return batch_offsets_.size();
}


void Metadata::SetNowOffset(int64_t offset) {
    now_offset_ = offset;
}

std::vector<PhysTypeVariant> Metadata::Serialize() const {
    int64_t old_offset = now_offset_;
    std::vector<PhysTypeVariant> result = schema_.Serialize();

    result.reserve(result.size() + columns_cnt_.size() * 3 + 2);

    result.push_back(static_cast<int64_t>(columns_cnt_.size()));

    for (size_t i = 0; i < columns_cnt_.size(); i++) {
        result.push_back(batch_offsets_[i]);
        result.push_back(columns_cnt_[i]);
        result.push_back(rows_cnt_[i]);
    }

    result.push_back(old_offset);

    return result;
}

}  // namespace cngn