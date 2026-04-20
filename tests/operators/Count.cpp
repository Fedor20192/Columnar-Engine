#include "Count.h"

#include <algorithm>
#include <memory>

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "BatchedWriter.h"
#include "CsvWriter.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "Simple Count", "[Count Operator]") {
    DefaultTestConfig::DefaultPrepare();

    auto context = std::make_shared<cngn::Context>(std::vector<std::string>{"a"});

    std::unique_ptr<cngn::Operator> count = std::make_unique<cngn::Count>(
        std::make_unique<cngn::Scan>(DefaultTestConfig::kFilename, context), context);
    count->Open();
    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch->ColumnCount() == 1);
    REQUIRE(count_batch.value()[0] == cngn::Column(cngn::ArrayType<cngn::Type::UInt64>{3}));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
    count->Close();
}

TEST_CASE_METHOD(GlogFixture, "Two batches Count", "[Count Operator]") {
    auto schema = DefaultTestConfig::kDefaultSchema;

    auto batch1 = DefaultTestConfig::k_default_batch;

    cngn::Batch batch2(std::vector{cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6}),
                                   cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6}),
                                   cngn::Column(std::vector<std::string_view>{
                                       "first", "second", "third", "fourth", "fifth", "sixth"}),
                                   cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6})},

                       schema);

    cngn::BatchedWriter writer(DefaultTestConfig::kFilename, schema);
    writer.WriteBatch(batch1);
    writer.WriteBatch(batch2);
    writer.WriteMetadata();
    writer.Flush();

    auto context = std::make_shared<cngn::Context>();

    std::unique_ptr<cngn::Operator> count = std::make_unique<cngn::Count>(
        std::make_unique<cngn::Scan>(DefaultTestConfig::kFilename, context), context);
    count->Open();

    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch->ColumnCount() == 1);
    REQUIRE(count_batch.value()[0] == cngn::Column(cngn::ArrayType<cngn::Type::UInt64>{3 + 6}));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
    count->Close();
}

TEST_CASE_METHOD(GlogFixture, "Some columns Count", "[Count Operator]") {
    DefaultTestConfig::DefaultPrepare();

    auto context = std::make_shared<cngn::Context>(std::vector<std::string>({"b"}));

    std::unique_ptr<cngn::Operator> count =
        std::make_unique<cngn::Count>(std::make_unique<cngn::Scan>(DefaultTestConfig::kFilename, context), context);
    count->Open();
    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch->ColumnCount() == 1);
    REQUIRE(count_batch.value()[0] == cngn::Column(cngn::ArrayType<cngn::Type::UInt64>{3}));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
    count->Close();
}