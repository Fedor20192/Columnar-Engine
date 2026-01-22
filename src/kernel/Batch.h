#pragma once

#include <vector>

#include "Column.h"
#include "Schema.h"
#include "glog/logging.h"

namespace cngn {

class Batch {
public:
    explicit Batch(const std::vector<Column>& columns, const Schema& schema)
        : columns_(columns), schema_(schema) {
        if (columns.size() != schema.GetData().size()) {
            DLOG(FATAL) << "You are trying to make batch with wrong schema" << std::endl;
            throw std::invalid_argument("Wrong number of batch columns");
        }
    }

    size_t ColumnCount() const {
        return columns_.size();
    }

    bool Empty() const {
        return columns_.empty();
    }

    const Column& operator[](size_t index) const {
        return columns_[index];
    }

private:
    std::vector<Column> columns_;
    Schema schema_;
};

}  // namespace cngn