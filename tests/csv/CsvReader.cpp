#include "catch2/catch_test_macros.hpp"
#include "CsvReader.h"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

using Row = cngn::CsvReader::Row;

void StringsToCsv(const std::string &filename, const std::vector<std::string> &lines) {
    std::ofstream ofs("tmp/" + filename);

    if (!ofs.is_open()) {
        throw std::runtime_error("Could not open file for writing");
    }

    for (const auto &line : lines) {
        ofs << line << '\n';
    }
}

std::vector<Row> StringsFromReader(const std::string &filename) {
    std::vector<Row> rows;
    cngn::CsvReader reader("tmp/" + filename);

    while (true) {
        std::optional<Row> row = reader.ReadLine();
        if (!row.has_value()) {
            break;
        }

        rows.push_back(std::move(row.value()));
    }

    return rows;
}

TEST_CASE("Empty File", "[CSVReader]") {
    std::string filename("example.csv");
    StringsToCsv(filename, std::vector<std::string>());

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{});

    // Проверим, что ридер и правда возвращает nullopt, когда строки кончились
    cngn::CsvReader reader("tmp/" + filename);
    std::optional<Row> row = reader.ReadLine();
    REQUIRE(!row.has_value());
}

TEST_CASE("Simple Read", "[CSV Reader]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {
                               "1,2,first,4",
                               "5,1,second,2",
                               "8,17,third,2",
                           });

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"1", "2", "first", "4"},
                                    {"5", "1", "second", "2"},
                                    {"8", "17", "third", "2"}});
}

TEST_CASE("Some empty fields", "[CSVReader][empty fields]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {"a,"
                            ",c,"});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"a", "", "c", ""}});
}

TEST_CASE("Multiple empty fields", "[CSVReader][empty fields]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {",,,"});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"", "", "", ""}});
}

TEST_CASE("Spaces are preserved", "[CSVReader][spaces]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {" a , b ,c ,d,  e "});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{" a ", " b ", "c ", "d", "  e "}});
}

TEST_CASE("Quoted field containing commas", "[CSVReader][quotes]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {R"("a,with,commas",b)"});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"a,with,commas", "b"}});
}

TEST_CASE("Escaped quotes inside quoted field", "[CSVReader][quotes]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {R"("He said ""hello""",world)"});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans == std::vector<Row>{{"He said \"hello\"", "world"}});
}

TEST_CASE("Multiline field inside quotes", "[CSVReader][multiline]") {
    std::string filename("example.csv");
    StringsToCsv(filename, {"1,2,3", R"("multi
line",end)",
                            "last,row"});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"1", "2", "3"});
    REQUIRE(ans[1] == Row{std::string("multi\nline"), "end"});
    REQUIRE(ans[2] == Row{"last", "row"});
}

TEST_CASE("Wikipedia test", "[CSVReader]") {
    std::string filename("example.csv");

    StringsToCsv(filename,
                 {R"(1997,Ford,E350,"ac, abs, moon",3000.00)",
                  R"(1999,Chevy,"Venture ""Extended Edition""", ,4900.00)",
                  R"(1996,Jeep,Grand Cherokee,"MUST SELL! air, moon roof, loaded",4799.00)"});

    auto ans = StringsFromReader(filename);
    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == Row{"1997", "Ford", "E350", "ac, abs, moon", "3000.00"});
    REQUIRE(ans[1] == Row{"1999", "Chevy", "Venture \"Extended Edition\"", " ", "4900.00"});
    REQUIRE(ans[2] ==
            Row{"1996", "Jeep", "Grand Cherokee", "MUST SELL! air, moon roof, loaded", "4799.00"});
}