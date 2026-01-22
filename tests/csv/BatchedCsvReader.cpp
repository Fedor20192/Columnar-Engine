#include "catch2/catch_template_test_macros.hpp"
#include "BatchedCsvReader.h"
#include "../Fixtures.h"
#include "../utils/StringCsvConverter.h"

#include <optional>
#include <string>

using Row = cngn::CsvReader::Row;
using Column = cngn::BatchedCsvReader::Column;
using Batch = cngn::BatchedCsvReader::Batch;

TEST_CASE_METHOD(GlogFixture, "Empty batch", "[Batched CSV Reader]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, std::vector<std::string>());

    auto ans = Converter::BatchFromCsv(filename);
    REQUIRE(ans == std::nullopt);
}

TEST_CASE_METHOD(GlogFixture, "Simple Batch", "[Batched CSV Reader]") {
    std::string filename("example.csv");
    std::vector<std::string> data = {
        "1,2,first,4",
        "5,1,second,2",
        "8,17,third,2",
    };
    Converter::StringsToCsv(filename, data);

    auto ans = Converter::BatchFromCsv(filename, data.size());
    REQUIRE(ans.has_value());
    REQUIRE(
        ans.value() ==
        Batch{{"1", "5", "8"}, {"2", "1", "17"}, {"first", "second", "third"}, {"4", "2", "2"}});
}

TEST_CASE_METHOD(GlogFixture, "Read a little more", "[Batched CSV Reader]") {
    std::string filename("example.csv");
    std::vector<std::string> data = {
        "1,2,first,4",
        "5,1,second,2",
        "8,17,third,2",
    };
    Converter::StringsToCsv(filename, data);

    auto ans = Converter::BatchFromCsv(filename, 100000);
    REQUIRE(ans.has_value());
    REQUIRE(
        ans.value() ==
        Batch{{"1", "5", "8"}, {"2", "1", "17"}, {"first", "second", "third"}, {"4", "2", "2"}});
}

TEST_CASE_METHOD(GlogFixture, "Different rows size", "[Batched CSV Reader]") {
    std::string filename("example.csv");
    std::vector<std::string> data = {
        "1,2,first,4",
        "5,1,second,2",
        "8,17,third,2",
        "14,fourth,87",
    };
    Converter::StringsToCsv(filename, data);

    auto ans = Converter::BatchFromCsv(filename, data.size());
    REQUIRE(ans == std::nullopt);
}