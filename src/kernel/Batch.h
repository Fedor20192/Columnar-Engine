#pragma once

#include <string>
#include <vector>

#include "Column.h"
#include "Schema.h"

namespace cngn {

class Batch {
public:
    Batch() = default;
    Batch(const Batch&) = delete;
    Batch& operator=(const Batch&) = delete;
    Batch(Batch&&) noexcept = default;
    Batch& operator=(Batch&&) noexcept = default;

    explicit Batch(const std::vector<Column>& columns, const Schema& schema);

    using Row = std::vector<std::string>;
    explicit Batch(const std::vector<Row>& rows, const Schema& schema);

    size_t ColumnCount() const;
    size_t RowCount() const;

    bool Empty() const;

    const Column& operator[](size_t index) const;
    const Column& GetColumnByName(const std::string& column_name) const;

    void AddColumn(Column&& column);

    std::vector<Row> Serialize() const;

private:
    std::vector<Column> columns_;
    Schema schema_;
};

}  // namespace cngn