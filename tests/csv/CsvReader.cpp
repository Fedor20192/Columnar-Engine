#include "catch2/catch_test_macros.hpp"
#include "CsvReader.h"
#include "../Fixtures.h"
#include "../utils/StringCsvConverter.h"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

using Row = cngn::CsvReader::Row;

TEST_CASE_METHOD(GlogFixture, "Empty File", "[CSVReader]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, std::vector<std::string>());

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.empty());

    // Проверим, что ридер и правда возвращает nullopt, когда строки кончились
    cngn::CsvReader reader(filename);
    std::optional<Row> row = reader.ReadLine();
    REQUIRE(!row.has_value());
}

TEST_CASE_METHOD(GlogFixture, "Simple Read", "[CSV Reader]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {
                                          "1,2,first,4",
                                          "5,1,second,2",
                                          "8,17,third,2",
                                      });

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"1", "2", "first", "4"},
                                    {"5", "1", "second", "2"},
                                    {"8", "17", "third", "2"}});
}

TEST_CASE_METHOD(GlogFixture, "Some empty fields", "[CSVReader][empty fields]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {"a,"
                                       ",c,"});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"a", "", "c", ""}});
}

TEST_CASE_METHOD(GlogFixture, "Multiple empty fields", "[CSVReader][empty fields]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {",,,"});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"", "", "", ""}});
}

TEST_CASE_METHOD(GlogFixture, "Spaces are preserved", "[CSVReader][spaces]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {" a , b ,c ,d,  e "});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{" a ", " b ", "c ", "d", "  e "}});
}

TEST_CASE_METHOD(GlogFixture, "Quoted field containing commas", "[CSVReader][quotes]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {R"("a,with,commas",b)"});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"a,with,commas", "b"}});
}

TEST_CASE_METHOD(GlogFixture, "Escaped quotes inside quoted field", "[CSVReader][quotes]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {R"("He said ""hello""",world)"});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"He said \"hello\"", "world"}});
}

TEST_CASE_METHOD(GlogFixture, "Multiline field inside quotes", "[CSVReader][multiline]") {
    std::string filename("example.csv");
    Converter::StringsToCsv(filename, {"1,2,3", R"("multi
line",end)",
                                       "last,row"});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"1", "2", "3"});
    REQUIRE(ans[1] == Row{std::string("multi\nline"), "end"});
    REQUIRE(ans[2] == Row{"last", "row"});
}

TEST_CASE_METHOD(GlogFixture, "Wikipedia test", "[CSVReader]") {
    std::string filename("example.csv");

    Converter::StringsToCsv(
        filename, {R"(1997,Ford,E350,"ac, abs, moon",3000.00)",
                   R"(1999,Chevy,"Venture ""Extended Edition""", ,4900.00)",
                   R"(1996,Jeep,Grand Cherokee,"MUST SELL! air, moon roof, loaded",4799.00)"});

    auto ans = Converter::StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"1997", "Ford", "E350", "ac, abs, moon", "3000.00"});
    REQUIRE(ans[1] == Row{"1999", "Chevy", "Venture \"Extended Edition\"", " ", "4900.00"});
    REQUIRE(ans[2] ==
            Row{"1996", "Jeep", "Grand Cherokee", "MUST SELL! air, moon roof, loaded", "4799.00"});
}