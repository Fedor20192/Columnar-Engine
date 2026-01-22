#pragma once

#include "CsvReader.h"

#include <optional>
#include <vector>

namespace cngn {

class BatchedCsvReader {
public:
    explicit BatchedCsvReader(const std::string& filename);

    using Column = std::vector<std::string>;

    std::optional<std::vector<Column>> ReadBatch(size_t batch_size);

private:
    CsvReader row_reader_;
};
}  // namespace cngn
