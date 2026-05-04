#include "Schema.h"

#include <string>
#include <vector>

#include "../Fixtures.h"
#include "CsvWriter.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "ReadSchema", "[BatchedReader]") {
    std::string schema_filename("schema.csv");
    std::vector<Row> schema_data = {
        {"a", "int64"},
        {"b", "int64"},
        {"name123", "string"},
        {"d", "int64"},
    };

    cngn::CsvWriter writer(schema_filename);
    writer.WriteAllRows(schema_data);
    writer.Flush();

    cngn::Schema schema(cngn::Schema::ReadFromCsv(schema_filename));

    REQUIRE(schema.GetData() ==
            std::vector<cngn::Schema::ColumnData>{{"a", cngn::Type::Int64},
                                                  {"b", cngn::Type::Int64},
                                                  {"name123", cngn::Type::String},
                                                  {"d", cngn::Type::Int64}});
}