#pragma once

#include <string>

#include "Batch.h"
#include "CsvWriter.h"
#include "Metadata.h"

namespace cngn {
class BatchedCsvWriter {
public:
    using Parameters = CsvWriter::Parameters;

    explicit BatchedCsvWriter(const std::string& filename, const Schema& schema,
                              Parameters parameters = Parameters());

    void WriteBatch(const Batch& batch);

    void WriteMetadata();

    void Flush();

private:
    CsvWriter row_writer_;
    Metadata metadata_;
};
}  // namespace cngn
