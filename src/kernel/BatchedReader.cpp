#include "BatchedReader.h"

#include "glog/logging.h"

namespace cngn {
BatchedReader::BatchedReader(const std::string &filename) : file_(filename, std::ios::binary) {
    DLOG(INFO) << "Constructing BatchedReader....\n";

    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open file " + filename + ".");
    }

    DLOG(INFO) << "BatchedReader trying read metadata\n";
    metadata_ = Metadata(ReadMetadata(file_));
    DLOG(INFO) << "BatchedReader read metadata!\n";

    uint64_t columns_cnt = metadata_.GetColumnsCnt();
    column_indices_.resize(columns_cnt);
    for (uint64_t i = 0; i < columns_cnt; i++) {
        column_indices_[i] = i;
    }

    DLOG(INFO) << "Constructed BatchedReader!\n";
}

void BatchedReader::SetIndices(std::vector<uint64_t> &&column_indices) {
    num_of_batch_ = 0;
    column_indices_ = std::move(column_indices);
}

std::optional<Batch> BatchedReader::ReadBatch() {
    DLOG(INFO) << "BatchedReader trying read batch number " << num_of_batch_ << "\n";
    if (num_of_batch_ >= metadata_.GetBatchCnt()) {
        DLOG(ERROR) << "Num of batch is too much: " << num_of_batch_ << " >= "
                    << metadata_.GetBatchCnt() << "\n";
        return std::nullopt;
    }
    file_.seekg(metadata_.GetOffsets()[num_of_batch_], std::ios::beg);
    uint64_t rows_cnt = metadata_.GetRowsCnt()[num_of_batch_];

    Batch batch(metadata_.GetSchema());
    size_t i = 0;
    uint64_t columns_cnt = metadata_.GetColumnsCnt();
    for (uint64_t column_index = 0; column_index < columns_cnt; column_index++) {
        while (i < column_indices_.size() && column_indices_[i] < column_index) {
            i++;
        }
        Type column_type = metadata_.GetSchema()[column_index].column_type;

        auto read_column = [this]<Type type>(uint32_t cnt) {
            Column::OwningPtr ptr;
            auto read = Reader().operator()<PhysicalType<type>>(file_, cnt, ptr);
            return Column(std::move(read), ptr);
        };

        auto skip_column = [this]<Type type>(uint32_t cnt) {
            using PT = PhysicalType<type>;

            if constexpr (Reader::IsIntegral<PT>()) {
                file_.seekg(sizeof(PT) * cnt, std::ios::cur);
            } else {
                auto offsets_start_pos = file_.tellg();
                uint32_t first_offset, last_offset;
                file_.read(reinterpret_cast<char *>(&first_offset), sizeof(uint32_t));
                file_.seekg(offsets_start_pos +
                            static_cast<std::streampos>(cnt * sizeof(uint32_t)));

                file_.read(reinterpret_cast<char *>(&last_offset), sizeof(uint32_t));

                uint32_t strings_size = last_offset - first_offset;

                file_.seekg(strings_size, std::ios::cur);
            }
        };

        if (i < column_indices_.size() && column_indices_[i] == column_index) {
            DLOG(INFO) << "Batched Reader trying read column number " << column_index << " from batch number " << num_of_batch_ << "\n";
            batch.AddColumn(DispatchOnType(column_type, read_column, rows_cnt));
        } else {
            DispatchOnType(column_type, skip_column, rows_cnt);
        }
    }

    DLOG(INFO) << "BatchedReader read batch number " << num_of_batch_ << "!\n";

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

}  // namespace cngn