#pragma once

#include <vector>

#include "Schema.h"
#include "Types.h"

namespace cngn {
class Metadata {
public:
    Metadata() = default;
    explicit Metadata(const Schema& schema);
    explicit Metadata(Metadata&& other) noexcept;
    Metadata& operator=(Metadata&& other) noexcept;

    explicit Metadata(Schema schema, std::vector<int64_t> batch_offsets,
                      std::vector<int64_t> columns_cnt, std::vector<int64_t> rows_cnt);

    const Schema& GetSchema() const;

    const std::vector<int64_t>& GetOffsets() const;

    const std::vector<int64_t>& GetColumnsCnt() const;

    const std::vector<int64_t>& GetRowsCnt() const;

    void AddBatch(size_t offset, size_t columns, size_t rows);

    int64_t GetNowOffset() const;

    size_t GetBatchSize() const;

    void SetNowOffset(int64_t offset);

    std::vector<PhysTypeVariant> Serialize() const;

private:
    Schema schema_;
    std::vector<int64_t> batch_offsets_, columns_cnt_, rows_cnt_;
    size_t now_offset_{0};
};
}  // namespace cngn