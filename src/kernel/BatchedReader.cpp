#include "BatchedReader.h"

#include "glog/logging.h"

namespace cngn {
BatchedReader::BatchedReader(const std::string &filename) : file_(filename, std::ios::binary) {
    if (!file_.is_open()) {
        DLOG(FATAL) << "Batched reader cannot open file " << filename << '\n';
        throw std::runtime_error("Cannot open file " + filename + ".");
    }
    metadata_ = Metadata(ReadMetadata(file_));
}

void BatchedReader::InitReading(const std::optional<std::vector<uint64_t>> &column_indices) {
    num_of_batch_ = 0;
    column_indices_ = column_indices;
}

std::optional<Batch> BatchedReader::ReadBatch() {
    if (num_of_batch_ >= metadata_.GetBatchCnt()) {
        return std::nullopt;
    }
    uint64_t rows_cnt = metadata_.GetRowsCnt()[num_of_batch_];
    uint64_t columns_cnt = metadata_.GetColumnsCnt();

    Batch batch(metadata_.GetSchema());
    for (uint64_t column_index = 0; column_index < columns_cnt; column_index++) {
        Type column_type = metadata_.GetSchema()[column_index].column_type;

        auto read_column = [this]<Type type>(uint64_t cnt) {
            ArrayType<type> array;
            array.reserve(cnt);

            for (uint64_t i = 0; i < cnt; i++) {
                array.emplace_back(Reader().operator()<PhysicalType<type>>(file_));
            }
            return Column(std::move(array));
        };

        batch.AddColumn(DispatchOnType(column_type, read_column, rows_cnt));
    }
    num_of_batch_++;
    return batch;
}

const Metadata &BatchedReader::GetMetadata() const {
    return metadata_;
}

Metadata BatchedReader::ReadMetadata(std::ifstream &in) {
    in.seekg(-sizeof(uint64_t), std::ios::end);

    Reader reader;

    uint64_t meta_offset = reader.operator()<uint64_t>(in);

    in.seekg(meta_offset);

    Schema schema = ReadSchema(in);

    uint64_t batch_cnt = reader.operator()<uint64_t>(in);

    std::vector<uint64_t> batch_offsets;
    std::vector<uint64_t> rows_cnt;
    batch_offsets.reserve(batch_cnt);
    rows_cnt.reserve(batch_cnt);

    for (uint64_t i = 0; i < batch_cnt; i++) {
        batch_offsets.push_back(reader.operator()<uint64_t>(in));
        rows_cnt.push_back(reader.operator()<uint64_t>(in));
    }

    in.seekg(0, std::ios::beg);

    return Metadata(std::move(schema), std::move(batch_offsets), std::move(rows_cnt));
}

Schema BatchedReader::ReadSchema(std::ifstream &in) {
    Reader reader;
    uint64_t size = reader.operator()<uint64_t>(in);

    std::vector<Schema::ColumnData> data;
    data.reserve(size);

    for (uint64_t i = 0; i < size; i++) {
        auto name = reader.operator()<std::string>(in);
        Type type = DeserializeType(reader.operator()<std::string>(in));
        data.emplace_back(std::move(name), type);
    }

    return Schema(std::move(data));
}

PhysTypeVariant BatchedReader::ReadElem(Type type) {
    return DispatchOnPhysType(type, Reader(), file_);
}

}  // namespace cngn