#include "Sort.h"

#include "../Fixtures.h"
#include "BatchedWriter.h"
#include "Expression.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

static const std::string kFilename = "sort_test.chsv";

TEST_CASE_METHOD(GlogFixture, "Simple ascending sort", "[Sort operator]") {
    cngn::Schema schema({
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int32},
    });

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{5, 3, 8, 1, 4}),
            cngn::Column(std::vector<int32_t>{10, 20, 30, 40, 50}),
        },
        schema);

    cngn::BatchedWriter writer(kFilename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto sort = std::make_unique<cngn::operators::Sort>(
        std::make_unique<cngn::operators::Scan>(kFilename, schema),
        std::vector<cngn::operators::SortKey>{
            {std::make_shared<cngn::operators::SelectExpression>("a"), "a"},
        });

    sort->Open();
    auto ans = sort->Next().value_or(nullptr);
    REQUIRE_FALSE(sort->Next());
    sort->Close();

    REQUIRE(ans);
    REQUIRE(ans->RowCount() == 5);
    REQUIRE((*ans)[0] == cngn::Column(std::vector<int64_t>{1, 3, 4, 5, 8}));
    REQUIRE((*ans)[1] == cngn::Column(std::vector<int32_t>{40, 20, 50, 10, 30}));
}

TEST_CASE_METHOD(GlogFixture, "Simple descending sort", "[Sort operator]") {
    cngn::Schema schema({
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int32},
    });

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{5, 3, 8, 1, 4}),
            cngn::Column(std::vector<int32_t>{10, 20, 30, 40, 50}),
        },
        schema);

    cngn::BatchedWriter writer(kFilename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto sort = std::make_unique<cngn::operators::Sort>(
        std::make_unique<cngn::operators::Scan>(kFilename, schema),
        std::vector<cngn::operators::SortKey>{
            {std::make_shared<cngn::operators::SelectExpression>("a"), "a"},
        },
        /*is_high_order=*/false);

    sort->Open();
    auto ans = sort->Next().value_or(nullptr);
    REQUIRE_FALSE(sort->Next());
    sort->Close();

    REQUIRE(ans);
    REQUIRE(ans->RowCount() == 5);
    REQUIRE((*ans)[0] == cngn::Column(std::vector<int64_t>{8, 5, 4, 3, 1}));
    REQUIRE((*ans)[1] == cngn::Column(std::vector<int32_t>{30, 10, 50, 20, 40}));
}

TEST_CASE_METHOD(GlogFixture, "Sort across two batches", "[Sort operator]") {
    cngn::Schema schema({
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::String},
    });

    cngn::Batch batch_1(
        std::vector{
            cngn::Column(std::vector<int64_t>{3, 1, 4}),
            cngn::Column(std::vector<std::string_view>{"aboba", "bebra", "aaaaaa"}),
        },
        schema);

    cngn::Batch batch_2(
        std::vector{
            cngn::Column(std::vector<int64_t>{1, 5, 9, 2}),
            cngn::Column(std::vector<std::string_view>{"a", "b", "c", "d"}),
        },
        schema);

    cngn::BatchedWriter writer(kFilename, schema);
    writer.WriteBatch(batch_1);
    writer.WriteBatch(batch_2);
    writer.WriteMetadata();
    writer.Flush();

    auto sort = std::make_unique<cngn::operators::Sort>(
        std::make_unique<cngn::operators::Scan>(kFilename, schema),
        std::vector<cngn::operators::SortKey>{
            {std::make_shared<cngn::operators::SelectExpression>("b"), "b"},
        });

    sort->Open();
    auto ans = sort->Next().value_or(nullptr);
    REQUIRE_FALSE(sort->Next());
    sort->Close();

    REQUIRE(ans);
    REQUIRE(ans->RowCount() == 7);
    REQUIRE((*ans)[0] == cngn::Column(std::vector<int64_t>{1, 4, 3, 5, 1, 9, 2}));
    REQUIRE((*ans)[1] == cngn::Column(std::vector<std::string_view>{"a", "aaaaaa", "aboba", "b", "bebra", "c", "d"}));
}

TEST_CASE_METHOD(GlogFixture, "Multi-column sort key", "[Sort operator]") {
    cngn::Schema schema({
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int32},
    });

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{2, 1, 2, 1}),
            cngn::Column(std::vector<int32_t>{4, 3, 2, 1}),
        },
        schema);

    cngn::BatchedWriter writer(kFilename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto sort = std::make_unique<cngn::operators::Sort>(
        std::make_unique<cngn::operators::Scan>(kFilename, schema),
        std::vector<cngn::operators::SortKey>{
            {std::make_shared<cngn::operators::SelectExpression>("a"), "a"},
            {std::make_shared<cngn::operators::SelectExpression>("b"), "b"},
        });

    sort->Open();
    auto ans = sort->Next().value_or(nullptr);
    REQUIRE_FALSE(sort->Next());
    sort->Close();

    REQUIRE(ans);
    REQUIRE(ans->RowCount() == 4);
    REQUIRE((*ans)[0] == cngn::Column(std::vector<int64_t>{1, 1, 2, 2}));
    REQUIRE((*ans)[1] == cngn::Column(std::vector<int32_t>{1, 3, 2, 4}));
}
