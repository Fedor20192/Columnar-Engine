#include "BatchedReader.h"

#include "glog/logging.h"

namespace cngn {
BatchedReader::BatchedReader(const std::string &filename) : file_(filename, std::ios::binary) {
    if (!file_.is_open()) {
        DLOG(FATAL) << "Batched reader cannot open file " << filename << '\n';
        throw std::runtime_error("Cannot open file " + filename + ".");
    }
    metadata_ = Metadata(ReadMetadata(file_));
    uint64_t columns_cnt = metadata_.GetColumnsCnt();
    column_indices_.resize(columns_cnt);
    for (uint64_t i = 0; i < columns_cnt; i++) {
        column_indices_[i] = i;
    }
}

void BatchedReader::SetIndices(std::vector<uint64_t> &&column_indices) {
    num_of_batch_ = 0;
    column_indices_ = std::move(column_indices);
}

std::optional<Batch> BatchedReader::ReadBatch() {
    if (num_of_batch_ >= metadata_.GetBatchCnt()) {
        return std::nullopt;
    }
    file_.seekg(metadata_.GetOffsets()[num_of_batch_], std::ios::beg);
    uint64_t rows_cnt = metadata_.GetRowsCnt()[num_of_batch_];

    Batch batch(metadata_.GetSchema());
    size_t i = 0;
    uint64_t columns_cnt = metadata_.GetColumnsCnt();
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

        auto column = DispatchOnType(column_type, read_column, rows_cnt);
        while (i < column_indices_.size() && column_indices_[i] < column_index) {
            i++;
        }
        if (i < column_indices_.size() && column_indices_[i] == column_index) {
            batch.AddColumn(std::move(column));
        }
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

    if (!batch_offsets.empty()) {
        in.seekg(batch_offsets[0], std::ios::beg);
    } else {
        in.seekg(0, std::ios::beg);
    }

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