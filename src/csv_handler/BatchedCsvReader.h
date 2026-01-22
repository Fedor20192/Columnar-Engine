#pragma once

#include "CsvReader.h"

#include <optional>
#include <vector>

namespace cngn {

class BatchedCsvReader {
public:
    explicit BatchedCsvReader(const std::string& filename);

    using Column = std::vector<std::string>;
    using Batch = std::vector<Column>;

    std::optional<Batch> ReadBatch(size_t batch_size);

private:
    CsvReader row_reader_;
};
}  // namespace cngn
