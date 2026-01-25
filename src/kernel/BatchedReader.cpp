#include "BatchedReader.h"

#include "glog/logging.h"

namespace cngn {
BatchedReader::BatchedReader(const std::string &filename) : file_(filename, std::ios::binary) {
    if (!file_.is_open()) {
        DLOG(FATAL) << "Batched reader cannot open file " << filename << std::endl;
        throw std::runtime_error("Cannot open file " + filename + ".");
    }
    metadata_ = Metadata(ReadMetadata(file_));
}

std::optional<Batch> BatchedReader::ReadBatch(size_t num_of_batch) {
    if (num_of_batch >= metadata_.GetBatchCnt()) {
        DLOG(ERROR) << "ReadBatch num_of_batch=" << num_of_batch << " > " << metadata_.GetBatchCnt()
                    << std::endl;
        return std::nullopt;
    }

    int64_t offset = metadata_.GetOffsets()[num_of_batch];
    file_.seekg(offset);

    int64_t rows_cnt = metadata_.GetRowsCnt()[num_of_batch];
    int64_t columns_cnt = metadata_.GetColumnsCnt()[num_of_batch];

    Batch batch(metadata_.GetSchema());
    for (int64_t column_index = 0; column_index < columns_cnt; column_index++) {
        Type column_type = metadata_.GetSchema().GetData()[column_index].column_type;

        switch (column_type) {
            case Type::Int64:
                batch.AddColumn(Column(ReadColumn<Type::Int64>(rows_cnt)));
                break;
            case Type::String:
                batch.AddColumn(Column(ReadColumn<Type::String>(rows_cnt)));
                break;
            default:
                DLOG(FATAL) << "Unknown column type" << std::endl;
                throw std::runtime_error("Unknown column type");
        }
    }

    return batch;
}

const Metadata &BatchedReader::GetMetadata() const {
    return metadata_;
}

Metadata BatchedReader::ReadMetadata(std::ifstream &in) {
    in.seekg(-sizeof(int64_t), std::ios::end);

    int64_t meta_offset = Read<int64_t>(in);

    in.seekg(meta_offset);

    Schema schema = ReadSchema(in);

    int64_t batch_cnt = Read<int64_t>(in);

    std::vector<int64_t> batch_offsets;
    std::vector<int64_t> columns_cnt;
    std::vector<int64_t> rows_cnt;
    batch_offsets.reserve(batch_cnt);
    columns_cnt.reserve(batch_cnt);
    rows_cnt.reserve(batch_cnt);

    for (int64_t i = 0; i < batch_cnt; i++) {
        batch_offsets.push_back(Read<int64_t>(in));
        columns_cnt.push_back(Read<int64_t>(in));
        rows_cnt.push_back(Read<int64_t>(in));
    }

    in.seekg(0, std::ios::beg);

    return Metadata(std::move(schema), std::move(batch_offsets), std::move(columns_cnt),
                    std::move(rows_cnt));
}

Schema BatchedReader::ReadSchema(std::ifstream &in) {
    int64_t size = Read<int64_t>(in);

    std::vector<Schema::ColumnData> data;
    data.reserve(size);

    for (int64_t i = 0; i < size; i++) {
        auto name = Read<std::string>(in);
        Type type = DeserializeType(Read<std::string>(in));
        data.emplace_back(std::move(name), type);
    }

    return Schema(std::move(data));
}

PhysTypeVariant BatchedReader::ReadElem(Type type) {
    switch (type) {
        case Type::Int64:
            return Read<int64_t>(file_);
        case Type::String:
            return Read<std::string>(file_);
        default:
            DLOG(FATAL) << "Unknown type" << std::endl;
            throw std::runtime_error("Unknown element type");
    }
}

}  // namespace cngn