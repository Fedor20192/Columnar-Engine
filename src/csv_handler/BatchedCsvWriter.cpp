#include "BatchedCsvWriter.h"
#include "glog/logging.h"

namespace cngn {
BatchedCsvWriter::BatchedCsvWriter(const std::string& filename, Parameters parameters)
    : row_writer_(filename, parameters) {
}

void BatchedCsvWriter::WriteBatch(const Batch& batch, size_t lines_to_write) {
    if (batch.empty()) {
        DLOG(ERROR) << "Trying to write empty batch" << std::endl;
        return;
    }

    CsvWriter::Row row(batch.size());
    for (size_t i = 0; i < lines_to_write; ++i) {
        for (size_t j = 0; j < batch.size(); ++j) {
            if (batch[j].size() < lines_to_write) {
                DLOG(ERROR) << "Column " << j << " is too short: " << batch[j].size() << " < "
                            << lines_to_write << std::endl;
                return;
            }
            row[j] = batch[j][i];
        }
        row_writer_.WriteRow(row);
    }
}

}  // namespace cngn