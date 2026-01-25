#include "Batch.h"

#include "glog/logging.h"

namespace cngn {

Batch::Batch(const Schema& schema) : schema_(schema) {
}

Batch::Batch(const std::vector<Column>& columns, const Schema& schema)
    : columns_(columns), schema_(schema) {
    if (columns.size() != schema.GetData().size()) {
        DLOG(FATAL) << "You are trying to make batch with wrong schema" << std::endl;
        throw std::invalid_argument("Wrong number of batch columns");
    }

    for (size_t i = 1; i < columns_.size(); ++i) {
        if (columns_[0].Size() != columns_[i].Size()) {
            DLOG(FATAL) << "Batch column size mismatch: " << columns_[0].Size()
                        << " != " << columns_[i].Size() << std::endl;
            throw std::invalid_argument("Batch column size mismatch");
        }
    }
}

Batch::Batch(const std::vector<Row>& rows, const Schema& schema)
    : schema_(schema) {
    if (rows.empty()) {
        return;
    }

    const size_t rows_count = rows.size(), columns_count = rows[0].size();

    if (columns_count > schema.GetColumnsCount()) {
        DLOG(FATAL) << "Columns count mismatch: " << columns_count
                    << " > " << schema.GetColumnsCount() << std::endl;
        throw std::invalid_argument("Batch num_of_batch mismatch");
    }

    columns_.reserve(columns_count);

    for (size_t column_index = 0; column_index < columns_count; ++column_index) {
        auto get_column = [&]<Type type>() {
            std::vector<PhysicalType<type>> arr;
            arr.reserve(rows_count);
            for (size_t row_index = 0; row_index < rows_count; ++row_index) {
                if (column_index >= rows[row_index].size()) {
                    DLOG(FATAL) << "Batch column index mismatch: " << column_index
                                << " != " << rows[row_index].size() << std::endl;
                    throw std::invalid_argument("Batch column index mismatch");
                }
                arr.push_back(Deserialize<type>(rows[row_index][column_index]));
            }
            return Column(arr);
        };

        columns_.push_back(DispatchOnType(schema[column_index].column_type, get_column));
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
        DLOG(ERROR) << "Bad column size: " << columns_[0].Size() << "!=" << column.Size()
                    << std::endl;
        throw std::invalid_argument("Wrong number of batch columns");
    }

    columns_.push_back(std::move(column));
}

std::vector<Batch::Row> Batch::Serialize() const {
    std::vector<Row> result;

    if (Empty()) {
        return result;
    }

    const size_t rows_count = columns_[0].Size();
    result.resize(rows_count);
    for (size_t i = 0; i < rows_count; ++i) {
        result[i].reserve(columns_.size());
    }

    for (size_t column_index = 0; column_index < columns_.size(); ++column_index) {
        for (size_t row_index = 0; row_index < rows_count; ++row_index) {
            result[row_index].push_back(columns_[column_index][row_index].ToString());
        }
    }

    return result;
}

}  // namespace cngn