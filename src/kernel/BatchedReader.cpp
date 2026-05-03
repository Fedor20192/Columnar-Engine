#include "BatchedReader.h"

#include "glog/logging.h"

namespace cngn {
BatchedReader::BatchedReader(const std::string &filename) : file_(filename, std::ios::binary) {
    DLOG(INFO) << "[BatchedReader]: Constructing BatchedReader....\n";

    if (!file_.is_open()) {
        throw std::runtime_error("[BatchedReader]: Cannot open file " + filename + ".");
    }

    DLOG(INFO) << "[BatchedReader]: BatchedReader trying read metadata\n";
    metadata_ = Metadata(ReadMetadata(file_));
    new_schema_ = metadata_.GetSchema();
    DLOG(INFO) << "[BatchedReader]: BatchedReader read metadata!\n";

    uint64_t columns_cnt = metadata_.GetColumnsCnt();
    column_indices_.resize(columns_cnt);
    for (uint64_t i = 0; i < columns_cnt; i++) {
        column_indices_[i] = i;
    }

    DLOG(INFO) << "[BatchedReader]: Constructed BatchedReader!\n";
}

void BatchedReader::SetIndices(std::vector<uint64_t> &&column_indices) {
    num_of_batch_ = 0;
    column_indices_ = std::move(column_indices);

    const auto &schema = metadata_.GetSchema();

    std::vector<Schema::ColumnData> new_schema_data(column_indices_.size());
    for (size_t i = 0; i < column_indices_.size(); i++) {
        const auto &[col_name, col_type] = schema[column_indices_[i]];
        new_schema_data[i] = {col_name, col_type};
    }

    new_schema_ = Schema(new_schema_data);
}

std::optional<Batch> BatchedReader::ReadBatch() {
    DLOG(INFO) << "[BatchedReader]: Trying read batch number " << num_of_batch_ << "\n";
    if (num_of_batch_ >= metadata_.GetBatchCnt()) {
        DLOG(ERROR) << "[BatchedReader]: Num of batch is too much: " << num_of_batch_
                    << " >= " << metadata_.GetBatchCnt() << "\n";
        return std::nullopt;
    }
    file_.seekg(metadata_.GetBatchesOffsets()[num_of_batch_], std::ios::beg);
    uint64_t rows_cnt = metadata_.GetRowsCnt()[num_of_batch_];

    DLOG(INFO) << "[BatchedReader]: Batch number " << num_of_batch_ << " has " << rows_cnt
               << " rows\n";

    Batch batch(new_schema_);

    if (!std::is_sorted(column_indices_.begin(), column_indices_.end())) {
        DLOG(WARNING) << "[BatchedReader]: Column indices is not sorted!\n";
    }

    for (uint64_t column_index : column_indices_) {
        Type column_type = metadata_.GetSchema()[column_index].column_type;

        auto read_column = [this]<Type type>(uint32_t cnt) {
            Column::OwningPtr ptr;
            auto read = Reader().operator()<PhysicalType<type>>(file_, cnt, ptr);
            return Column(std::move(read), ptr);
        };

        DLOG(INFO) << "[BatchedReader]: Batched Reader trying read column number " << column_index
                   << " from batch number " << num_of_batch_
                   << "\n"
                      "Offset = "
                   << metadata_.GetColumnOffset(num_of_batch_, column_index);
        file_.seekg(metadata_.GetColumnOffset(num_of_batch_, column_index), std::ios::beg);
        batch.AddColumn(DispatchOnType(column_type, read_column, rows_cnt));
    }

    DLOG(INFO) << "[BatchedReader]: BatchedReader read batch number " << num_of_batch_ << "!\n";

    num_of_batch_++;
    return batch;
}

const Metadata &BatchedReader::GetMetadata() const {
    return metadata_;
}

Metadata BatchedReader::ReadMetadata(std::ifstream &in) {
    DLOG(INFO) << "[BatchedReader]: Starting read metadata\n";

    in.seekg(-sizeof(uint64_t), std::ios::end);

    Reader reader;

    uint64_t meta_offset = reader.operator()<uint64_t>(in);

    DLOG(INFO) << "[BatchedReader]: Meta offset = " << meta_offset << '\n';

    in.seekg(meta_offset);

    Schema schema = ReadSchema(in);

    uint64_t batch_cnt = reader.operator()<uint64_t>(in);
    uint64_t total_columns = reader.operator()<uint64_t>(in);

    DLOG(INFO) << "[BatchedReader]: Batch count = " << batch_cnt << "\n"
               << "Total columns count = " << total_columns << "\n";

    std::vector<uint64_t> batch_offsets;
    std::vector<uint64_t> rows_cnt;
    std::vector<uint64_t> column_offsets;
    batch_offsets.reserve(batch_cnt);
    rows_cnt.reserve(batch_cnt);
    column_offsets.reserve(total_columns);

    for (uint64_t i = 0; i < batch_cnt; i++) {
        batch_offsets.push_back(reader.operator()<uint64_t>(in));
        rows_cnt.push_back(reader.operator()<uint64_t>(in));
    }

    for (uint64_t i = 0; i < total_columns; i++) {
        column_offsets.push_back(reader.operator()<uint64_t>(in));
    }

    if (!batch_offsets.empty()) {
        in.seekg(batch_offsets[0], std::ios::beg);
    } else {
        in.seekg(0, std::ios::beg);
    }

    DLOG(INFO) << "[BatchedReader]: Metadata successfully read!\n";

    return Metadata(std::move(schema), std::move(batch_offsets), std::move(rows_cnt),
                    std::move(column_offsets));
}

Schema BatchedReader::ReadSchema(std::ifstream &in) {
    DLOG(INFO) << "[BatchedReader]: Starting read schema\n";

    Reader reader;
    uint64_t size = reader.operator()<uint64_t>(in);

    DLOG(INFO) << "[BatchedReader]: Schema size = " << size << "\n";

    std::vector<Schema::ColumnData> data;
    data.reserve(size);

    for (uint64_t i = 0; i < size; i++) {
        auto name = reader.operator()<std::string>(in);
        Type type = DeserializeType(reader.operator()<std::string>(in));
        data.emplace_back(std::move(name), type);
    }

    DLOG(INFO) << "[BatchedReader]: Schema successfully read!\n";

    return Schema(std::move(data));
}

}  // namespace cngn