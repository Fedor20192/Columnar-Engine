#pragma once

#include <vector>

#include "Schema.h"

namespace cngn {
class Metadata {
public:
    explicit Metadata(const Schema& schema) : schema_(schema) {
    }

    explicit Metadata(const Schema& schema, const std::vector<size_t>& batch_offsets,
                      const std::vector<size_t>& batch_sizes)
        : schema_(schema), batch_offsets_(batch_offsets), rows_cnt_(batch_sizes) {
    }

    const Schema& GetSchema() const {
        return schema_;
    }

    const std::vector<size_t>& GetOffsets() const {
        return batch_offsets_;
    }

    const std::vector<size_t>& GetSizes() const {
        return rows_cnt_;
    }

    void AddBatch(size_t offset, size_t rows) {
        batch_offsets_.push_back(now_offset_);
        now_offset_ = offset;
        rows_cnt_.push_back(rows);
    }

    size_t GetNowOffset() const {
        return now_offset_;
    }

    void SetNowOffset(size_t offset) {
        now_offset_ = offset;
    }

    using Row = Schema::Row;
    std::vector<Row> Serialize() const {
        std::vector<Row> result = schema_.Serialize();

        result.reserve(result.size() + rows_cnt_.size() + 1);

        result.push_back({std::to_string(rows_cnt_.size())});

        for (size_t i = 0; i < rows_cnt_.size(); i++) {
            result.push_back({std::to_string(batch_offsets_[i]), std::to_string(rows_cnt_[i])});
        }

        return result;
    }

private:
    Schema schema_;
    std::vector<size_t> batch_offsets_, rows_cnt_;
    size_t now_offset_{0};
};
}  // namespace cngn