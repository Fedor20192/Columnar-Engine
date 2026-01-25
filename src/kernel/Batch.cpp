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

}  // namespace cngn