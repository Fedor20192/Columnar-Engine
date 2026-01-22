#pragma once

#include "CsvWriter.h"

#include <string>

namespace cngn {
class BatchedCsvWriter {
public:
    using Column = std::vector<std::string>;
    using Batch = std::vector<Column>;
    using Parameters = CsvWriter::Parameters;

    explicit BatchedCsvWriter(const std::string &filename, Parameters parameters = Parameters());

    void WriteBatch(const Batch& batch, size_t lines_to_write);

private:
    CsvWriter row_writer_;
};
}  // namespace cngn
