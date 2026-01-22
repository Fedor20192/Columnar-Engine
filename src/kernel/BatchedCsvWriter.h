#pragma once

#include "Batch.h"
#include "CsvWriter.h"

#include <string>

namespace cngn {
class BatchedCsvWriter {
public:
    using Parameters = CsvWriter::Parameters;

    explicit BatchedCsvWriter(const std::string &filename, Parameters parameters = Parameters());

    void WriteBatch(const Batch& batch, size_t lines_to_write);

private:
    CsvWriter row_writer_;
};
}  // namespace cngn
