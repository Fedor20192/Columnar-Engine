#include "Count.h"

#include <algorithm>
#include <memory>

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "BatchedWriter.h"
#include "Filter.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Simple Count", "[Count Operator]") {
    DefaultTestConfig::DefaultPrepare();

    auto count = std::make_unique<cngn::operators::Count>(std::make_unique<cngn::operators::Scan>(
        DefaultTestConfig::kFilename, cngn::Schema({{"a", cngn::Type::Int64}})));
    count->Open();
    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch.value()->ColumnCount() == 1);
    REQUIRE((*count_batch.value())[0] == cngn::Column(cngn::ArrayType<cngn::Type::UInt64>{3}));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
    count->Close();
}

TEST_CASE_METHOD(GlogFixture, "Two batches Count", "[Count Operator]") {
    auto schema = DefaultTestConfig::kDefaultSchema;

    auto batch1 = cngn::Batch(
        std::vector{cngn::Column(std::vector<int64_t>{1, 5, 8}),
                    cngn::Column(std::vector<int64_t>{2, 1, 17}),
                    cngn::Column(std::vector<std::string_view>{"first", "second", "third"}),
                    cngn::Column(std::vector<int64_t>{4, 2, 2})},
        DefaultTestConfig::kDefaultSchema);

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

    auto count = std::make_unique<cngn::operators::Count>(
        std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, cngn::Schema()));
    count->Open();

    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch.value()->ColumnCount() == 1);
    REQUIRE((*count_batch.value())[0] == cngn::Column(cngn::ArrayType<cngn::Type::UInt64>{3 + 6}));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
    count->Close();
}

TEST_CASE_METHOD(GlogFixture, "Some columns Count", "[Count Operator]") {
    DefaultTestConfig::DefaultPrepare();

    auto count = std::make_unique<cngn::operators::Count>(
        std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, cngn::Schema()));
    count->Open();
    auto count_batch = count->Next();

    REQUIRE(count_batch.has_value());
    REQUIRE(count_batch.value()->ColumnCount() == 1);
    REQUIRE((*count_batch.value())[0] == cngn::Column(cngn::ArrayType<cngn::Type::UInt64>{3}));

    count_batch = count->Next();
    REQUIRE(!count_batch.has_value());
    count->Close();
}

TEST_CASE_METHOD(GlogFixture, "Strict Filter", "[Count Operator]") {

    cngn::Schema schema({
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int16},
        {"c", cngn::Type::String},
        {"d", cngn::Type::Int32},
    });

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6, 7, 8}),
            cngn::Column(std::vector<int16_t>{0, 0, 0, 0, 0, 0, 0, 0}),
            cngn::Column(std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"}),
            cngn::Column(std::vector<int32_t>{14, 22, 8, 88, 69, 67, 0, 1}),
        },
        schema);

    const std::string filename = "filter.chsv";
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto filter =
        std::make_unique<cngn::operators::Count>(std::make_unique<cngn::operators::Filter>(
            std::make_unique<cngn::operators::Scan>(filename, cngn::Schema({
                                                                  {"b", cngn::Type::Int16},
                                                                  {"c", cngn::Type::String},
                                                                  {"d", cngn::Type::Int32},
                                                              })),
            std::make_shared<cngn::operators::BinaryExpression>(
                cngn::operators::BinaryExpressionType::Neq,
                std::make_shared<cngn::operators::SelectExpression>("b"),
                std::make_shared<cngn::operators::ConstantExpression>(static_cast<int16_t>(0)))));

    filter->Open();

    auto ans_op = filter->Next();

    filter->Close();

    REQUIRE(ans_op.has_value());

    auto ans = ans_op.value();
    const auto& count_batch = *ans;

    REQUIRE(ans->RowCount() == 1);
    REQUIRE(ans->ColumnCount() == 1);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>(count_batch[0][0]) == 0);
}