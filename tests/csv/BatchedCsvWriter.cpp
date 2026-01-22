#include "BatchedCsvWriter.h"
#include "catch2/catch_template_test_macros.hpp"
#include "../utils/StringCsvConverter.h"
#include "../Fixtures.h"

using Row = cngn::CsvWriter::Row;
using Batch = cngn::BatchedCsvWriter::Batch;

TEST_CASE_METHOD(GlogFixture, "Write Empty Batch", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch;
    Converter::BatchToCsv(filename, batch);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.empty());
}

TEST_CASE_METHOD(GlogFixture, "Write Simple Batch", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch = {
        {"1", "5", "8"},
        {"2", "1", "17"},
        {"first", "second", "third"},
        {"4", "2", "2"},
    };
    Converter::BatchToCsv(filename, batch, 3);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"1", "2", "first", "4"});
    REQUIRE(ans[1] == Row{"5", "1", "second", "2"});
    REQUIRE(ans[2] == Row{"8", "17", "third", "2"});
}

TEST_CASE_METHOD(GlogFixture, "Write Single Row Batch", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch = {
        {"col1", "col2", "col3"},
    };
    Converter::BatchToCsv(filename, batch, 3);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"col1"});
    REQUIRE(ans[1] == Row{"col2"});
    REQUIRE(ans[2] == Row{"col3"});
}

TEST_CASE_METHOD(GlogFixture, "Write Single Column Batch", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch = {
        {"row1"},
        {"row2"},
        {"row3"},
    };
    Converter::BatchToCsv(filename, batch, 1);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == Batch{{"row1", "row2", "row3"}});
}

TEST_CASE_METHOD(GlogFixture, "Write Batch with Empty Strings", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch = {
        {"", "value", ""},
        {"data", "", "test"},
    };
    Converter::BatchToCsv(filename, batch, 3);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"", "data"});
    REQUIRE(ans[1] == Row{"value", ""});
    REQUIRE(ans[2] == Row{"", "test"});
}

TEST_CASE_METHOD(GlogFixture, "Write Large Batch", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch;

    for (int i = 0; i < 1000; ++i) {
        Row row;
        row.reserve(10);
        for (int j = 0; j < 10; ++j) {
            row.push_back("value_" + std::to_string(i) + "_" + std::to_string(j));
        }
        batch.push_back(row);
    }

    Converter::BatchToCsv(filename, batch, 10);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 10);

    REQUIRE(ans[0][0] == "value_0_0");
    REQUIRE(ans[0][999] == "value_999_0");
    REQUIRE(ans[9][0] == "value_0_9");
    REQUIRE(ans[9][999] == "value_999_9");
}

TEST_CASE_METHOD(GlogFixture, "Write Batch with Different Row Lengths", "[BatchedCSVWriter]") {
    std::string filename("example.csv");
    Batch batch = {
        {"a", "b", "c"},
        {"d", "e"},
        {"f", "g", "h", "i"},
    };

    Converter::BatchToCsv(filename, batch, 4);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.empty());
}

TEST_CASE_METHOD(GlogFixture, "Overwrite Existing File", "[BatchedCSVWriter]") {
    std::string filename("example.csv");

    Batch batch1 = {{"old1", "old2"}, {"old3", "old4"}, {"old5", "old6"}};
    Converter::BatchToCsv(filename, batch1, 3);

    Batch batch2 = {{"new1", "new2"}, {"new3", "new4"}};
    Converter::BatchToCsv(filename, batch2, 2);

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 2);
    REQUIRE(ans[0] == Row{"new1", "new3"});
    REQUIRE(ans[1] == Row{"new2", "new4"});
}

