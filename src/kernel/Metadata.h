#pragma once

#include <vector>

#include "Schema.h"
#include "Types.h"

namespace cngn {
class Metadata {
public:
    Metadata() = default;
    explicit Metadata(const Schema& schema);
    explicit Metadata(Metadata&& other) noexcept = default;
    Metadata& operator=(Metadata&& other) noexcept = default;

    explicit Metadata(Schema schema, std::vector<uint64_t> batch_offsets,
                      std::vector<uint64_t> rows_cnt, std::vector<uint64_t> columns_offsets);

    const Schema& GetSchema() const;

    const std::vector<uint64_t>& GetBatchesOffsets() const;

    uint64_t GetColumnOffset(uint64_t batch_index, uint64_t column_index) const;

    uint64_t GetColumnsCnt() const;

    const std::vector<uint64_t>& GetRowsCnt() const;

    void AddBatch(size_t offset, size_t rows, std::vector<uint64_t> &&columns_offsets);

    uint64_t GetNowOffset() const;

    size_t GetBatchCnt() const;

    void SetNowOffset(uint64_t offset);

    std::vector<PhysTypeVariant> Serialize() const;

private:
    Schema schema_;
    std::vector<uint64_t> batch_offsets_, rows_cnt_;
    std::vector<uint64_t> columns_offsets_;
    uint64_t now_offset_{0};
};
}  // namespace cngn