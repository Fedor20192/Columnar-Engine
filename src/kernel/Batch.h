#pragma once

#include <string>
#include <vector>

#include "Column.h"
#include "CsvReader.h"
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

    explicit Batch(CsvReader::Chunk&& chunk, const Schema& schema, size_t rows_count);

    size_t ColumnCount() const;

    bool Empty() const;

    const Column& operator[](size_t index) const;

    void AddColumn(Column&& column);

    std::vector<std::vector<std::string>> Serialize() const;

private:
    std::vector<Column> columns_;
    std::shared_ptr<std::vector<char>> buffer_;
};

}  // namespace cngn