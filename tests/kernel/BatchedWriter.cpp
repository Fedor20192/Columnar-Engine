#include <string>
#include <vector>

#include "../Fixtures.h"
#include "BatchedCsvWriter.h"
#include "Schema.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = std::vector<std::string>;

TEST_CASE_METHOD(GlogFixture, "Bebra", "[BatchedCSVWriter]") {
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

    cngn::Schema schema(cngn::Schema::ReadFromFile(schema_filename));

    std::string filename("example.csv");

    cngn::BatchedCsvWriter batched_writer(filename, schema);

    cngn::Batch batch(
        {
            cngn::Column({1, 5, 8}),
            cngn::Column({2, 1, 17}),
            cngn::Column({"first", "second", "third"}),
            cngn::Column({4, 2, 2}),
        },
        schema);

    batched_writer.WriteBatch(batch);

    batched_writer.WriteMetadata();
    batched_writer.Flush();

    cngn::CsvReader reader(filename);
    auto result = reader.ReadAllLines();
    REQUIRE(result.size() == 12);

    REQUIRE(result[0] == Row{"1", "5", "8"});
    REQUIRE(result[1] == Row{"2", "1", "17"});
    REQUIRE(result[2] == Row{"first", "second", "third"});
    REQUIRE(result[3] == Row{"4", "2", "2"});

    REQUIRE(result[4] == Row{"4"});
    REQUIRE(result[5] == Row{"a", "int64"});
    REQUIRE(result[6] == Row{"b", "int64"});
    REQUIRE(result[7] == Row{"name123", "string"});
    REQUIRE(result[8] == Row{"d", "int64"});

    REQUIRE(result[9] == Row{"1"});
    REQUIRE(result[10] == Row{"0", "3"});

    REQUIRE(result[11] == Row{"62"});
}