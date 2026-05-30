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
constexpr size_t kRowsInBatch = 8192, kBatchInQueue = 100, kChunksInQueue = 10;

void FromCsvToFormat(const std::string &schema_name, const std::string &source_name,
                     const std::string &table_name) {
    DLOG(INFO) << "[FromCsvToFormat]: Starting: \n"
                  "Schema name: "
               << schema_name << '\n'
               << "Source name: " << source_name << '\n'
               << "Table name: " << table_name << '\n';

    Queue<std::pair<CsvReader::Chunk, size_t>> chunks_queue(kChunksInQueue);
    Queue<Batch> batch_queue(kBatchInQueue);

    Schema schema = Schema::ReadFromCsv(schema_name);

    auto csv_reader = std::make_shared<CsvReader>(source_name);

    std::jthread reader([&] {
        while (true) {
            bool is_empty = true;
            size_t rows_count = 0;
            for (size_t num_of_row = 0; num_of_row < kRowsInBatch; num_of_row++) {
                if (!csv_reader->ReadLine()) {
                    break;
                }
                is_empty = false;
                rows_count++;
            }
            if (is_empty) {
                break;
            }
            chunks_queue.Push(std::make_pair(csv_reader->GetChunk(), rows_count));
        }
        chunks_queue.Close();
    });

    std::jthread writer([&] {
        BatchedWriter batched_writer(table_name + ".chsv", schema);

        while (auto batch = batch_queue.Pop()) {
            batched_writer.WriteBatch(std::move(batch.value()));
        }

        batched_writer.WriteMetadata();
    });

    while (true) {
        if (auto chunk_op = chunks_queue.Pop(); chunk_op.has_value()) {
            auto &[chunk, rows_count] = chunk_op.value();
            batch_queue.Push(Batch(std::move(chunk), schema, rows_count));
        } else {
            break;
        }
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