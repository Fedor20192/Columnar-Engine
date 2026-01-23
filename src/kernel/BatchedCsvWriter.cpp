#include "BatchedCsvWriter.h"

#include "glog/logging.h"

namespace cngn {
BatchedCsvWriter::BatchedCsvWriter(const std::string& filename, const Schema& schema,
                                   Parameters parameters)
    : row_writer_(filename, parameters), metadata_(schema) {
}


void BatchedCsvWriter::WriteBatch(const Batch& batch) {
    if (batch.Empty()) {
        DLOG(ERROR) << "Trying to write empty batch" << std::endl;
        return;
    }

    CsvWriter::Row row(batch.ColumnCount());

    for (size_t column_index = 0; column_index < batch.ColumnCount(); ++column_index) {
        const size_t row_cnt = batch[column_index].Size();
        CsvWriter::Row row(row_cnt);

        for (size_t row_index = 0; row_index < row_cnt; ++row_index) {
            row[row_index] = batch[column_index][row_index].ToString();
        }
        const size_t now_offset = row_writer_.WriteRow(row);
        metadata_.AddBatch(now_offset, row_cnt);
    }
}

void BatchedCsvWriter::WriteMetadata() {
    row_writer_.WriteAllRows(metadata_.Serialize());
}


}  // namespace cngn