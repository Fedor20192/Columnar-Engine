#include "CsvWriter.h"

#include <fstream>
#include <string>

#include "../Fixtures.h"
#include "../utils/StringCsvConverter.h"
#include "CsvReader.h"
#include "catch2/catch_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "Empty", "[CSVWriter]") {
    std::string filename("example.csv");
    std::vector<Row> rows;

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);

    cngn::CsvReader reader(filename);
    auto ans = reader.ReadAllLines();
    REQUIRE(ans.empty());
}

TEST_CASE_METHOD(GlogFixture, "Single row with one field", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Hello"}};

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == Row{"Hello"});
}

TEST_CASE_METHOD(GlogFixture, "Many fields one row", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Name", "Age", "City"}};

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == Row{"Name", "Age", "City"});
}

TEST_CASE_METHOD(GlogFixture, "Multiple rows", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Name", "Age", "City"},
                             {"Alice", "1488", "Birobidzhan"},
                             {"Bob", "69", "Tynda"},
                             {"Charlie", "228", "Penza"}};

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 4);

    REQUIRE(result[0] == Row{"Name", "Age", "City"});
    REQUIRE(result[1] == Row{"Alice", "1488", "Birobidzhan"});
    REQUIRE(result[2] == Row{"Bob", "69", "Tynda"});
    REQUIRE(result[3] == Row{"Charlie", "228", "Penza"});
}

TEST_CASE_METHOD(GlogFixture, "Empty fields", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Field1", "", "Field3"}, {"", "Value2", ""}, {"OnlyOne", "", ""}};

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == Row{"Field1", "", "Field3"});
    REQUIRE(result[1] == Row{"", "Value2", ""});
    REQUIRE(result[2] == Row{"OnlyOne", "", ""});
}

TEST_CASE_METHOD(GlogFixture, "Special characters in fields", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"Field, with comma", "Field with \"quotes\"", "Field with\nnewline"},
                             {"Normal", "Value", "Another"}};

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 2);
}

TEST_CASE_METHOD(GlogFixture, "Large", "[CSVWriter]") {
    std::string filename("example.csv");

    constexpr int kRowCount = 1000;
    std::vector<Row> rows;
    rows.reserve(kRowCount);

    for (int i = 0; i < kRowCount; ++i) {
        rows.push_back({"Row_" + std::to_string(i), "Value_" + std::to_string(i * 2),
                        "Data_" + std::to_string(i * 3)});
    }

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == kRowCount);
    REQUIRE(result[0][0] == "Row_0");
    REQUIRE(result[499][1] == "Value_998");
    REQUIRE(result[999][2] == "Data_2997");
}

TEST_CASE_METHOD(GlogFixture, "Whitespace handling", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows = {{"  Leading", "Trailing  ", "  Both  "},
                             {"Tab\tHere", "New\nLine", "Spaces   Here"}};

    cngn::CsvWriter writer(filename);
    writer.WriteAllRows(rows);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == Row{"  Leading", "Trailing  ", "  Both  "});
    REQUIRE(result[1] == Row{"Tab\tHere", "New\nLine", "Spaces   Here"});
}

TEST_CASE_METHOD(GlogFixture, "Append rows", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows1 = {{"First", "Batch"}};
    cngn::CsvWriter writer1(filename);
    writer1.WriteAllRows(rows1);
    writer1.Flush();

    std::vector<Row> rows2 = {{"Second", "Batch"}};
    writer1.WriteAllRows(rows2);
    writer1.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 2);
    REQUIRE(result[0][0] == "First");
}

TEST_CASE_METHOD(GlogFixture, "Append to existing file", "[CSVWriter]") {
    std::string filename("example.csv");

    std::vector<Row> rows1 = {{"First", "Batch"}};
    Converter::StringsToCsv(filename, rows1[0]);

    std::vector<Row> rows2 = {{"Second", "Batch"}};
    cngn::CsvWriter writer(filename);
    writer.WriteRow(rows2[0]);
    writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 1);
    REQUIRE(result[0][0] == "Second");
}