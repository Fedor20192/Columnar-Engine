#include "catch2/catch_test_macros.hpp"
#include "CsvReader.h"
#include "CsvWriter.h"

#include <fstream>
#include <string>

using Row = cngn::CsvWriter::Row;

void RowsToCsv(const std::string& filename, const std::vector<Row>& rows) {
    cngn::CsvWriter writer(filename);

    for (const auto& row : rows) {
        writer.WriteRow(row);
    }
}

std::vector<Row> StringsFromReader(const std::string &filename) {
    std::vector<Row> rows;
    cngn::CsvReader reader(filename);

    while (true) {
        std::optional<Row> row = reader.ReadLine();
        if (!row.has_value()) {
            break;
        }

        rows.push_back(std::move(row.value()));
    }

    return rows;
}

TEST_CASE("Empty", "[CSVWriter]") {
    std::string filename("example.csv");
    std::vector<Row> rows;
    RowsToCsv(filename, rows);

    auto ans = StringsFromReader(filename);
    REQUIRE(ans.empty());
}