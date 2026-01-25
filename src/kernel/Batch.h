#pragma once

#include <string>
#include <vector>

#include "Column.h"
#include "Schema.h"

namespace cngn {

class Batch {
public:
    explicit Batch(const Schema& schema);

    explicit Batch(const std::vector<Column>& columns, const Schema& schema);

    using Row = std::vector<std::string>;
    explicit Batch(const std::vector<Row>& rows, const Schema& schema);

    size_t ColumnCount() const;

    bool Empty() const;

    const Column& operator[](size_t index) const;

    void AddColumn(Column&& column);

    std::vector<Row> Serialize() const;

private:
    std::vector<Column> columns_;
    Schema schema_;
};

}  // namespace cngn