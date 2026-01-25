#pragma once

#include <vector>

#include "Column.h"
#include "Schema.h"

namespace cngn {

class Batch {
public:
    explicit Batch(const Schema& schema);

    explicit Batch(const std::vector<Column>& columns, const Schema& schema);

    size_t ColumnCount() const;

    bool Empty() const;

    const Column& operator[](size_t index) const;

    void AddColumn(Column&& column);

private:
    std::vector<Column> columns_;
    Schema schema_;
};

}  // namespace cngn