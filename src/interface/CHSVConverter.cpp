#include "CHSVConverter.h"

#include <optional>
#include <thread>

#include "BatchedReader.h"
#include "BatchedWriter.h"
#include "CsvReader.h"
#include "CsvWriter.h"
#include "Queue.h"
#include "glog/logging.h"

namespace cngn {
constexpr size_t kRowsInBatch = 65536, kBatchInQueue = 100, kRowsInQueue = kRowsInBatch * 10;

void FromCsvToFormat(const std::string &schema_name, const std::string &source_name,
                     const std::string &table_name) {
    DLOG(INFO) << "[FromCsvToFormat]: Starting: \n"
                  "Schema name: "
               << schema_name << '\n'
               << "Source name: " << source_name << '\n'
               << "Table name: " << table_name << '\n';

    Queue<CsvReader::Row> rows_queue(kRowsInQueue);
    Queue<Batch> batch_queue(kBatchInQueue);

    Schema schema = Schema::ReadFromCsv(schema_name);

    std::jthread reader([&] {
        CsvReader csv_reader(source_name);
        while (auto row = csv_reader.ReadLine()) {
            rows_queue.Push(std::move(row.value()));
        }
        rows_queue.Close();
    });

    std::jthread writer([&] {
        BatchedWriter batched_writer(table_name + ".chsv", schema);

        while (auto batch = batch_queue.Pop()) {
            batched_writer.WriteBatch(std::move(batch.value()));
        }

        batched_writer.WriteMetadata();
    });

    while (true) {
        bool is_empty = true;

        std::vector<CsvReader::Row> rows;
        rows.reserve(kRowsInBatch);
        for (size_t num_of_row = 0; num_of_row < kRowsInBatch; num_of_row++) {
            std::optional<CsvReader::Row> row = rows_queue.Pop();
            if (!row.has_value()) {
                break;
            }
            is_empty = false;
            rows.emplace_back(std::move(row.value()));
        }

        if (is_empty) {
            break;
        }

        batch_queue.Push(Batch(rows, schema));
    }

    batch_queue.Close();

    DLOG(INFO) << "[FromCsvToFormat]: Finished!\n";
}

void FromFormatToCsv(const std::string &table_name, const std::string &target_name) {
    CsvWriter csv_writer(target_name);
    BatchedReader batched_reader(table_name + ".chsv");

    while (std::optional<Batch> batch = batched_reader.ReadBatch()) {
        csv_writer.WriteAllRows(batch->Serialize());
    }
}
}  // namespace cngn