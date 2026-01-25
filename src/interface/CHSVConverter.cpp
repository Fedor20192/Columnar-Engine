#include "CHSVConverter.h"

#include <optional>

#include "BatchedReader.h"
#include "BatchedWriter.h"
#include "CsvReader.h"
#include "CsvWriter.h"

namespace cngn {
constexpr size_t kRowsInBatch = 100;

void FromCsvToFormat(const std::string &schema_name, const std::string &source_name,
                     const std::string &table_name) {
    CsvReader csv_reader(source_name);
    Schema schema = Schema::ReadFromCsv(schema_name);
    BatchedWriter batched_writer(table_name + ".chsv", schema);

    while (true) {
        bool is_empty = true;

        std::vector<CsvReader::Row> rows;
        rows.reserve(kRowsInBatch);
        for (size_t num_of_row = 0; num_of_row < kRowsInBatch; num_of_row++) {
            std::optional<CsvReader::Row> row = csv_reader.ReadLine();
            if (!row.has_value()) {
                break;
            }
            is_empty = false;
            rows.push_back(row.value());
        }

        if (is_empty) {
            break;
        }

        batched_writer.WriteBatch(Batch(rows, schema));
    }
    batched_writer.WriteMetadata();
}

void FromFormatToCsv(const std::string &table_name, const std::string &target_name) {
    CsvWriter csv_writer(target_name);
    BatchedReader batched_reader(table_name + ".chsv");

    size_t num_of_batch = 0;
    while (std::optional<Batch> batch = batched_reader.ReadBatch(num_of_batch++)) {
        csv_writer.WriteAllRows(batch->Serialize());
    }
}
}  // namespace cngn