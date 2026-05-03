#include "Batch.h"

#include "glog/logging.h"

namespace cngn {

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

Batch::Batch(const std::vector<Row>& rows, const Schema& schema) : schema_(schema) {
    if (rows.empty()) {
        return;
    }

    const size_t rows_count = rows.size(), columns_count = rows[0].size();

    if (columns_count > schema.GetColumnsCount()) {
        throw std::invalid_argument("[Batch]: Columns count mismatch " +
                                    std::to_string(columns_count) + " > " +
                                    std::to_string(schema.GetColumnsCount()) + '\n');
    }

    columns_.reserve(columns_count);

    for (size_t column_index = 0; column_index < columns_count; ++column_index) {
        auto get_column = [&]<Type type>() {
            std::vector<PhysicalType<type>> arr;
            arr.reserve(rows_count);

            if constexpr (type == Type::String) {
                size_t total_size = 0;
                for (const auto& row : rows) {
                    total_size += row[column_index].size();
                }

                auto buffer = std::make_shared<char[]>(total_size);
                char* current_ptr = buffer.get();

                for (size_t row_index = 0; row_index < rows_count; ++row_index) {
                    const std::string& s = rows[row_index][column_index];

                    std::memcpy(current_ptr, s.data(), s.size());

                    arr.emplace_back(current_ptr, s.size());

                    current_ptr += s.size();
                }
                return Column(std::move(arr), buffer);
            } else {
                for (size_t row_index = 0; row_index < rows_count; ++row_index) {
                    if (column_index >= rows[row_index].size()) {
                        throw std::invalid_argument("[Batch]: Batch column index mismatch: " +
                                                    std::to_string(column_index) + " != " +
                                                    std::to_string(rows[row_index].size()));
                    }
                    arr.emplace_back(Deserialize<type>(rows[row_index][column_index]));
                }
                return Column(std::move(arr), nullptr);
            }
        };

        columns_.emplace_back(DispatchOnType(schema[column_index].column_type, get_column));
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

std::vector<Batch::Row> Batch::Serialize() const {
    DLOG(INFO) << "[Batch]: Batch::Serialize()\n";

    std::vector<Row> result;

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