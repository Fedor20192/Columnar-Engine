#include "Scan.h"

#include <memory>

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "BatchedWriter.h"
#include "CsvWriter.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "Simple Scan", "[ScanOperator]") {
    auto batch = DefaultTestConfig::DefaultPrepare();

    std::unique_ptr<cngn::Operator> scan =
        std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, cngn::Schema({
            {"a", cngn::Type::Int64},
            {"b", cngn::Type::Int64},
            {"name123", cngn::Type::String},
            {"d", cngn::Type::Int64},
        }));
    scan->Open();

    auto file_batch = scan->Next();

    REQUIRE(file_batch.has_value());
    REQUIRE(file_batch.value()->ColumnCount() == batch.ColumnCount());

    for (size_t i = 0; i < batch.ColumnCount(); i++) {
        REQUIRE(batch[i] == (*file_batch.value())[i]);
    }

    file_batch = scan->Next();
    REQUIRE(!file_batch.has_value());
    scan->Close();
}

TEST_CASE_METHOD(GlogFixture, "Scan columns", "[ScanOperator]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"d", cngn::Type::Int64},
        {"name123", cngn::Type::String},
    }});

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{1, 5, 8}),
            cngn::Column(std::vector<int64_t>{2, 1, 17}),
            cngn::Column(std::vector<int64_t>{4, 2, 2}),
            cngn::Column(std::vector<std::string_view>{"first", "second", "third"}),
        },
        schema);

    cngn::BatchedWriter writer(DefaultTestConfig::kFilename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    std::unique_ptr<cngn::Operator> scan =
        std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, cngn::Schema({
            {"name123", cngn::Type::String},
            {"b", cngn::Type::Int64},
        }));
    scan->Open();

    auto file_batch = scan->Next();

    REQUIRE(file_batch.has_value());
    REQUIRE(file_batch.value()->ColumnCount() == 2);

    REQUIRE((*file_batch.value())[0] == batch[1]);
    REQUIRE((*file_batch.value())[1] == batch[3]);

    file_batch = scan->Next();
    REQUIRE(!file_batch.has_value());
    scan->Close();
}

TEST_CASE_METHOD(GlogFixture, "Scan bad names columns", "[ScanOperator]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"name123", cngn::Type::String},
    }});

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{1, 5, 8}),
            cngn::Column(std::vector<std::string_view>{"first", "second", "third"}),
        },
        schema);

    cngn::BatchedWriter writer(DefaultTestConfig::kFilename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto scan = std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, cngn::Schema({
        {"name123", cngn::Type::String},
        {"b", cngn::Type::Int64},
    }));

    REQUIRE_THROWS(scan->Open());
}