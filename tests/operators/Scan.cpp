#include "Scan.h"

#include <memory>

#include "../Fixtures.h"
#include "BatchedWriter.h"
#include "CsvWriter.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "Simple Scan", "[ScanOperator]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::Int64},
    }});

    cngn::Batch batch(schema);
    batch.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{2, 1, 17}));
    batch.AddColumn(cngn::Column(std::vector<std::string_view>{"first", "second", "third"}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{4, 2, 2}));

    std::string filename("test.chsv");
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto context = std::make_shared<cngn::Context>(std::vector<std::string>{"a", "b", "name123", "d"});

    std::unique_ptr<cngn::Operator> scan = std::make_unique<cngn::Scan>(filename, context);
    scan->Open();

    auto file_batch = scan->Next();

    REQUIRE(file_batch.has_value());
    REQUIRE(file_batch->ColumnCount() == batch.ColumnCount());

    for (size_t i = 0; i < batch.ColumnCount(); i++) {
        REQUIRE(batch[i] == file_batch.value()[i]);
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

    cngn::Batch batch(schema);
    batch.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{2, 1, 17}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{4, 2, 2}));
    batch.AddColumn(cngn::Column(std::vector<std::string_view>{"first", "second", "third"}));

    std::string filename("test.chsv");
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto context = std::make_shared<cngn::Context>(std::vector<std::string>{"name123", "b"});

    std::unique_ptr<cngn::Operator> scan = std::make_unique<cngn::Scan>(filename, context);
    scan->Open();

    auto file_batch = scan->Next();

    REQUIRE(file_batch.has_value());
    REQUIRE(file_batch->ColumnCount() == 2);

    REQUIRE(file_batch.value()[0] == batch[1]);
    REQUIRE(file_batch.value()[1] == batch[3]);

    file_batch = scan->Next();
    REQUIRE(!file_batch.has_value());
    scan->Close();
}

TEST_CASE_METHOD(GlogFixture, "Scan bad names columns", "[ScanOperator]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"name123", cngn::Type::String},
    }});

    cngn::Batch batch(schema);
    batch.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch.AddColumn(cngn::Column(std::vector<std::string_view>{"first", "second", "third"}));

    std::string filename("test.chsv");
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto context = std::make_shared<cngn::Context>(std::vector<std::string>{"name123", "b"});

    auto scan = std::make_unique<cngn::Scan>(filename, context);

    REQUIRE_THROWS(scan->Open());
}