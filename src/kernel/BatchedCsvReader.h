#pragma once

#include "CsvReader.h"

#include <optional>
#include <vector>

namespace cngn {

class BatchedCsvReader {
public:
    using Column = std::vector<std::string>;
    using Batch = std::vector<Column>;
    using Parameters = CsvReader::Parameters;

    explicit BatchedCsvReader(const std::string& filename, Parameters parameters = Parameters());

    std::optional<Batch> ReadBatch(size_t batch_size);

private:
    CsvReader row_reader_;
};
}  // namespace cngn
