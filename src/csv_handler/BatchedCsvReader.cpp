#include "BatchedCsvReader.h"
#include "glog/logging.h"

namespace cngn {
BatchedCsvReader::BatchedCsvReader(const std::string& filename) : row_reader_(filename) {
}

std::optional<std::vector<BatchedCsvReader::Column>> BatchedCsvReader::ReadBatch(
    size_t batch_size) {
    std::vector<Column> columns;

    std::optional<CsvReader::Row> row = row_reader_.ReadLine();

    if (row == std::nullopt) {
        return std::nullopt;
    }

    const size_t num_columns = row->size();
    columns.resize(num_columns);

    for (size_t i = 0; i < num_columns; ++i) {
        columns[i].reserve(batch_size);
        columns[i].push_back(std::move(row->at(i)));
    }

    for (size_t i = 1; i < batch_size; ++i) {
        row = row_reader_.ReadLine();
        if (row == std::nullopt) {
            break;
        }

        if (row->size() != num_columns) {
            DLOG(ERROR) << "Different number of columns in row" << std::endl;
            return std::nullopt;
        }

        for (size_t j = 0; j < num_columns; ++j) {
            columns[j].push_back(std::move(row->at(j)));
        }
    }

    return columns;
}

}  // namespace cngn