#include "../Fixtures.h"
#include "../utils/StringCsvConverter.h"
#include "CHSVConverter.h"
#include "CsvReader.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Converter Cross Validation", "[Converter]") {
    std::string scheme_filename("schema.csv"), data_filename("data.csv");

    std::vector<std::string> schema_data{
        R"(a,int64)",
        R"(b,int64)",
        R"(name123,string)",
        R"(d,int64)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(1,2,first,4)",
        R"(5,1,second,2)",
        R"(8,17,third,2)",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"1", "2", "first", "4"});
    REQUIRE(result[1] == cngn::CsvReader::Row{"5", "1", "second", "2"});
    REQUIRE(result[2] == cngn::CsvReader::Row{"8", "17", "third", "2"});
}

TEST_CASE_METHOD(GlogFixture, "Converter Empty CSV", "[Converter]") {
    std::string scheme_filename("empty_schema.csv"), data_filename("empty_data.csv");

    std::vector<std::string> schema_data{
        R"(id,int64)",
        R"(name,string)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data;
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("empty_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("empty_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.empty());
}

TEST_CASE_METHOD(GlogFixture, "Converter Single Column Int64", "[Converter]") {
    std::string scheme_filename("single_int_schema.csv"), data_filename("single_int_data.csv");

    std::vector<std::string> schema_data{
        R"(value,int64)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(1)", R"(2)", R"(3)", R"(100)", R"(-50)",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("single_int_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("single_int_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"1"});
    REQUIRE(result[1] == cngn::CsvReader::Row{"2"});
    REQUIRE(result[2] == cngn::CsvReader::Row{"3"});
    REQUIRE(result[3] == cngn::CsvReader::Row{"100"});
    REQUIRE(result[4] == cngn::CsvReader::Row{"-50"});
}

TEST_CASE_METHOD(GlogFixture, "Converter Single Column String", "[Converter]") {
    std::string scheme_filename("single_str_schema.csv"), data_filename("single_str_data.csv");

    std::vector<std::string> schema_data{
        R"(text,string)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(hello)",
        R"(world)",
        R"(test with spaces)",
        R"("""quoted, string""")",
        R"(very long string that should be handled correctly)",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("single_str_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("single_str_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"hello"});
    REQUIRE(result[1] == cngn::CsvReader::Row{"world"});
    REQUIRE(result[2] == cngn::CsvReader::Row{"test with spaces"});
    REQUIRE(result[3] == cngn::CsvReader::Row{"\"quoted, string\""});
    REQUIRE(result[4] == cngn::CsvReader::Row{"very long string that should be handled correctly"});
}

TEST_CASE_METHOD(GlogFixture, "Converter Mixed Types", "[Converter]") {
    std::string scheme_filename("mixed_schema.csv"), data_filename("mixed_data.csv");

    std::vector<std::string> schema_data{
        R"(id,int64)", R"(name,string)", R"(age,int64)", R"(city,string)", R"(salary,int64)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(1,John,25,New York,50000)",
        R"(2,Jane,30,London,60000)",
        R"(3,Bob,35,"""Paris, France""",70000)",
        R"(4,Alice,28,Tokyo,55000)",
        R"(5,Charlie,40,"""San Francisco, CA""",80000)",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("mixed_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("mixed_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"1", "John", "25", "New York", "50000"});
    REQUIRE(result[1] == cngn::CsvReader::Row{"2", "Jane", "30", "London", "60000"});
    REQUIRE(result[2] == cngn::CsvReader::Row{"3", "Bob", "35", "\"Paris, France\"", "70000"});
    REQUIRE(result[3] == cngn::CsvReader::Row{"4", "Alice", "28", "Tokyo", "55000"});
    REQUIRE(result[4] ==
            cngn::CsvReader::Row{"5", "Charlie", "40", "\"San Francisco, CA\"", "80000"});
}

TEST_CASE_METHOD(GlogFixture, "Converter All String Columns", "[Converter]") {
    std::string scheme_filename("all_string_schema.csv"), data_filename("all_string_data.csv");

    std::vector<std::string> schema_data{
        R"(first_name,string)",
        R"(last_name,string)",
        R"(email,string)",
        R"(phone,string)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(John,Doe,john@example.com,123-456-7890)",
        R"(Jane,Smith,jane.smith@test.com,987-654-3210)",
        R"(Bob,Johnson,bob123@gmail.com,555-123-4567)",
        R"("""Alex, Jr""",O'Brien,alex.obrien@company.com,(444) 888-9999)",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("all_string_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("all_string_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"John", "Doe", "john@example.com", "123-456-7890"});
    REQUIRE(result[1] ==
            cngn::CsvReader::Row{"Jane", "Smith", "jane.smith@test.com", "987-654-3210"});
    REQUIRE(result[2] ==
            cngn::CsvReader::Row{"Bob", "Johnson", "bob123@gmail.com", "555-123-4567"});
    REQUIRE(result[3] == cngn::CsvReader::Row{"\"Alex, Jr\"", "O'Brien", "alex.obrien@company.com",
                                              "(444) 888-9999"});
}

TEST_CASE_METHOD(GlogFixture, "Converter Large Int64 Values", "[Converter]") {
    std::string scheme_filename("large_int_schema.csv"), data_filename("large_int_data.csv");

    std::vector<std::string> schema_data{
        R"(tiny,int64)",
        R"(small,int64)",
        R"(large,int64)",
        R"(negative,int64)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(0,1000,1000000000,-1)",
        R"(1,9999,2147483647,-1000)",
        R"(255,65535,9223372036854775807,-9223372036854775808)",
        R"(-128,-32768,-1000000000000,0)",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("large_int_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("large_int_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"0", "1000", "1000000000", "-1"});
    REQUIRE(result[1] == cngn::CsvReader::Row{"1", "9999", "2147483647", "-1000"});
    REQUIRE(result[2] ==
            cngn::CsvReader::Row{"255", "65535", "9223372036854775807", "-9223372036854775808"});
    REQUIRE(result[3] == cngn::CsvReader::Row{"-128", "-32768", "-1000000000000", "0"});
}

TEST_CASE_METHOD(GlogFixture, "Converter Special Characters in Strings", "[Converter]") {
    std::string scheme_filename("special_chars_schema.csv"),
        data_filename("special_chars_data.csv");

    std::vector<std::string> schema_data{
        R"(text,string)",
        R"(more_text,string)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data{
        R"(line1,simple)",       R"("""quoted""",normal)",  R"("with,comma,inside",a)",
        R"(new\nline,tab\ttab)", R"(unicode®©™,emoji😀🎉)", R"("""escaped quotes""","back\\slash")",
    };
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("special_chars_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("special_chars_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());
    REQUIRE(result[0] == cngn::CsvReader::Row{"line1", "simple"});
    REQUIRE(result[1] == cngn::CsvReader::Row{"\"quoted\"", "normal"});
    REQUIRE(result[2] == cngn::CsvReader::Row{"with,comma,inside", "a"});
    REQUIRE(result[4] == cngn::CsvReader::Row{"unicode®©™", "emoji😀🎉"});
    REQUIRE(result[5] == cngn::CsvReader::Row{"\"escaped quotes\"", "back\\\\slash"});
}

TEST_CASE_METHOD(GlogFixture, "Converter Many Rows", "[Converter]") {
    std::string scheme_filename("many_rows_schema.csv"), data_filename("many_rows_data.csv");

    std::vector<std::string> schema_data{
        R"(index,int64)",
        R"(value,string)",
    };
    StringCSVConverter().StringsToCsv(scheme_filename, schema_data);

    std::vector<std::string> data;
    for (int i = 0; i < 1000; ++i) {
        data.push_back(std::to_string(i) + ",Row " + std::to_string(i));
    }
    StringCSVConverter().StringsToCsv(data_filename, data);

    std::string target_filename("many_rows_target.chsv");
    cngn::FromCsvToFormat(scheme_filename, data_filename, target_filename);

    std::string new_csv_filename("many_rows_new.csv");
    cngn::FromFormatToCsv(target_filename, new_csv_filename);

    cngn::CsvReader reader(new_csv_filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == data.size());

    REQUIRE(result[0] == cngn::CsvReader::Row{"0", "Row 0"});
    REQUIRE(result[499] == cngn::CsvReader::Row{"499", "Row 499"});
    REQUIRE(result[999] == cngn::CsvReader::Row{"999", "Row 999"});
}
