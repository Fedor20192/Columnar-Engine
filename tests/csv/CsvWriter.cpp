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

TEST_CASE("Single row with one field", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Hello"}};
    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == Row{"Hello"});
}

TEST_CASE("Many fields one row", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Name", "Age", "City"}};
    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == Row{"Name", "Age", "City"});
}

TEST_CASE("Multiple rows", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {
        {"Name", "Age", "City"},
        {"Alice", "1488", "Birobidzhan"},
        {"Bob", "69", "Tynda"},
        {"Charlie", "228", "Penza"}
    };

    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 4);

    REQUIRE(result[0] == Row{"Name", "Age", "City"});
    REQUIRE(result[1] == Row{"Alice", "1488", "Birobidzhan"});
    REQUIRE(result[2] == Row{"Bob", "69", "Tynda"});
    REQUIRE(result[3] == Row{"Charlie", "228", "Penza"});
}

TEST_CASE("Empty fields", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {
        {"Field1", "", "Field3"},
        {"", "Value2", ""},
        {"OnlyOne", "", ""}
    };

    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == Row{"Field1", "", "Field3"});
    REQUIRE(result[1] == Row{"", "Value2", ""});
    REQUIRE(result[2] == Row{"OnlyOne", "", ""});
}

TEST_CASE("Special characters in fields", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {
        {"Field, with comma", "Field with \"quotes\"", "Field with\nnewline"},
        {"Normal", "Value", "Another"}
    };

    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 2);
}

TEST_CASE("Large", "[CSVWriter]") {
    std::string filename("example.csv");

    constexpr int kRowCount = 1000;
    std::vector<Row> rows;
    rows.reserve(kRowCount);

    for (int i = 0; i < kRowCount; ++i) {
        rows.push_back({
            "Row_" + std::to_string(i),
            "Value_" + std::to_string(i * 2),
            "Data_" + std::to_string(i * 3)
        });
    }

    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == kRowCount);
    REQUIRE(result[0][0] == "Row_0");
    REQUIRE(result[499][1] == "Value_998");
    REQUIRE(result[999][2] == "Data_2997");
}

TEST_CASE("Whitespace handling", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {
        {"  Leading", "Trailing  ", "  Both  "},
        {"Tab\tHere", "New\nLine", "Spaces   Here"}
    };

    RowsToCsv(filename, rows);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == Row{"  Leading", "Trailing  ", "  Both  "});
    REQUIRE(result[1] == Row{"Tab\tHere", "New\nLine", "Spaces   Here"});
}

TEST_CASE("Append to existing file", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows1 = {{"First", "Batch"}};
    RowsToCsv(filename, rows1);

    std::vector<Row> rows2 = {{"Second", "Batch"}};
    RowsToCsv(filename, rows2);

    auto result = StringsFromReader(filename);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0][0] == "Second");
}