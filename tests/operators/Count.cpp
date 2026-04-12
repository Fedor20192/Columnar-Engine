#include "Count.h"

#include <algorithm>
#include <memory>

#include "../Fixtures.h"
#include "BatchedWriter.h"
#include "CsvWriter.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "Simple Count", "[Count Operator]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::Int64},
    }});

    cngn::Batch batch(schema);
    batch.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{2, 1, 17}));
    batch.AddColumn(cngn::Column(std::vector<std::string>{"first", "second", "third"}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{4, 2, 2}));

    std::string filename("test.chsv");
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    std::unique_ptr<cngn::Operator> count =
        std::make_unique<cngn::Count>(std::make_unique<cngn::Scan>(filename));

    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch->ColumnCount() == 1);
    REQUIRE(count_batch.value()[0] == cngn::Column(cngn::ArrayType<cngn::Type::Int64>(3)));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
}

TEST_CASE_METHOD(GlogFixture, "Two batches Count", "[Count Operator]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::Int64},
    }});

    cngn::Batch batch1(schema);
    batch1.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch1.AddColumn(cngn::Column(std::vector<int64_t>{2, 1, 17}));
    batch1.AddColumn(cngn::Column(std::vector<std::string>{"first", "second", "third"}));
    batch1.AddColumn(cngn::Column(std::vector<int64_t>{4, 2, 2}));

    cngn::Batch batch2(schema);
    batch2.AddColumn(cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6}));
    batch2.AddColumn(cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6}));
    batch2.AddColumn(cngn::Column(std::vector<std::string>{"first", "second", "third", "fourth", "fifth", "sixth"}));
    batch2.AddColumn(cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6}));

    std::string filename("test.chsv");
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch1);
    writer.WriteBatch(batch2);
    writer.WriteMetadata();
    writer.Flush();

    std::unique_ptr<cngn::Operator> count =
        std::make_unique<cngn::Count>(std::make_unique<cngn::Scan>(filename));

    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch->ColumnCount() == 1);
    REQUIRE(count_batch.value()[0] == cngn::Column(cngn::ArrayType<cngn::Type::Int64>(3 + 6)));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
}