#include "Batch.h"

#include "glog/logging.h"

namespace cngn {

Batch::Batch(const std::vector<Column>& columns, const Schema& schema) : columns_(columns) {
    DLOG(INFO) << "[Batch]: Trying construct batch\n";
    if (columns.size() != schema.GetData().size()) {
        throw std::invalid_argument(
            "[Batch]: You are trying to make batch with wrong schema.\n"
            "columns count != schema columns count:\n" +
            std::to_string(columns_.size()) + " != " + std::to_string(schema.GetData().size()));
    }

    for (size_t i = 1; i < columns_.size(); ++i) {
        if (columns_[0].Size() != columns_[i].Size()) {
            throw std::invalid_argument(
                "[Batch]: Batch column size mismatch: " + std::to_string(columns_[0].Size()) +
                " != " + std::to_string(columns_[i].Size()));
        }
    }

    DLOG(INFO) << "[Batch]: Batch successfully constructed!\n";
}

Batch::Batch(CsvReader::Chunk&& chunk, const Schema& schema, size_t rows_count) {
    if (chunk.Empty()) {
        return;
    }

    const size_t columns_count = chunk.GetColsCount(rows_count);

    if (columns_count > schema.GetColumnsCount()) {
        throw std::invalid_argument("[Batch]: Columns count mismatch " +
                                    std::to_string(columns_count) + " > " +
                                    std::to_string(schema.GetColumnsCount()) + '\n');
    }

    columns_.reserve(columns_count);
    buffer_ = chunk.GetBuffer();

    for (size_t column_index = 0; column_index < columns_count; ++column_index) {
        auto get_column = [&]<Type type>() {
            std::vector<PhysicalType<type>> arr;
            arr.reserve(rows_count);

            if constexpr (type == Type::String) {
                for (size_t row_index = 0; row_index < rows_count; ++row_index) {
                    const auto s = chunk.GetField(row_index, column_index, rows_count);
                    arr.emplace_back(s);
                }
                return Column(std::move(arr));
            } else {
                for (size_t row_index = 0; row_index < rows_count; ++row_index) {
                    arr.emplace_back(Deserialize<type>(chunk.GetField(row_index, column_index, rows_count)));
                }
                return Column(std::move(arr));
            }
        };

        columns_.emplace_back(DispatchOnType(schema[column_index].column_type, get_column));
    }
}

size_t Batch::ColumnCount() const {
    return columns_.size();
}

bool Batch::Empty() const {
    return columns_.empty();
}

const Column& Batch::operator[](size_t index) const {
    return columns_[index];
}

void Batch::AddColumn(Column&& column) {
    if (!Empty() && column.Size() != columns_[0].Size()) {
        throw std::invalid_argument(
            "[Batch]: Wrong number of batch columns: " + std::to_string(columns_[0].Size()) +
            " != " + std::to_string(column.Size()));
    }

    columns_.emplace_back(std::move(column));
}

std::vector<std::vector<std::string>> Batch::Serialize() const {
    DLOG(INFO) << "[Batch]: Batch::Serialize()\n";

    std::vector<std::vector<std::string>> result;

    if (Empty()) {
        DLOG(INFO) << "[Batch]: Batch::Serialized! Its empty!\n";
        return result;
    }

    const size_t rows_count = columns_[0].Size();
    result.resize(rows_count);
    for (size_t i = 0; i < rows_count; ++i) {
        result[i].reserve(columns_.size());
    }

    for (size_t column_index = 0; column_index < columns_.size(); ++column_index) {
        for (size_t row_index = 0; row_index < rows_count; ++row_index) {
            result[row_index].emplace_back(ToString(columns_[column_index][row_index]));
        }
    }

    DLOG(INFO) << "[Batch]: Batch::Serialized!\n";

    return result;
}

}  // namespace cngn