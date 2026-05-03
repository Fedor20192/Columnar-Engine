#include "Batch.h"

#include <functional>

#include "glog/logging.h"

namespace cngn {

Batch::Batch(const Schema& schema) : schema_(schema) {
}

Batch::Batch(const std::vector<Column>& columns, const Schema& schema)
    : columns_(columns), schema_(schema) {
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

Batch::Batch(CsvReader::Chunk&& chunk, const Schema& schema, size_t rows_count) : schema_(schema) {
    if (chunk.Empty()) {
        return;
    }

    chunk.InitColumnsCnt(rows_count);

    const size_t columns_count = chunk.GetColsCount(rows_count);

    if (columns_count > schema.GetColumnsCount()) {
        throw std::invalid_argument("[Batch]: Columns count mismatch " +
                                    std::to_string(columns_count) + " > " +
                                    std::to_string(schema.GetColumnsCount()) + '\n');
    }

    columns_.reserve(columns_count);

    using FieldParser = std::function<void(size_t row_index, size_t col_index)>;
    std::vector<FieldParser> parsers;
    parsers.reserve(columns_count);

    for (size_t i = 0; i < columns_count; i++) {
        columns_.emplace_back(schema[i].column_type, rows_count);
        auto prepare_parser = [this, &chunk]<Type type>() -> FieldParser {
            return [this, &chunk](size_t row_ind, size_t col_ind) {
                auto field = chunk.GetField(row_ind, col_ind);
                columns_[col_ind].PushBack<type>(Deserialize<type>(field));
            };
        };
        parsers.push_back(DispatchOnType(schema[i].column_type, prepare_parser));
    }

    buffer_ = chunk.GetBuffer();

    for (size_t row_index = 0; row_index < rows_count; ++row_index) {
        for (size_t column_index = 0; column_index < columns_.size(); ++column_index) {
            parsers[column_index](row_index, column_index);
        }
    }
}

size_t Batch::ColumnCount() const {
    return columns_.size();
}

size_t Batch::RowCount() const {
    if (columns_.empty()) {
        return 0;
    }
    return columns_[0].Size();
}

bool Batch::Empty() const {
    return columns_.empty();
}

const Column& Batch::operator[](size_t index) const {
    return columns_[index];
}

const Column& Batch::GetColumnByName(const std::string& column_name) const {
    const auto& fields = schema_.GetData();

    auto it = std::find_if(fields.begin(), fields.end(),
                           [&column_name](const Schema::ColumnData& column_data) {
                               return column_name == column_data.column_name;
                           });

    if (it != fields.end()) {
        return columns_[it - fields.begin()];
    }

    throw std::invalid_argument("[Batch]: Column " + column_name + " not found!");
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

const Schema& Batch::GetSchema() const {
    return schema_;
}

}  // namespace cngn