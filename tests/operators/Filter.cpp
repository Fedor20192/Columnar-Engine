#include "Filter.h"

#include <memory>

#include "../Fixtures.h"
#include "BatchedWriter.h"
#include "Count.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Simple Filter", "[Filter Operator]") {

    cngn::Schema schema({
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int16},
        {"c", cngn::Type::String},
        {"d", cngn::Type::Int32},
    });

    cngn::Batch batch(
        std::vector{
            cngn::Column(std::vector<int64_t>{1, 2, 3, 4, 5, 6, 7, 8}),
            cngn::Column(std::vector<int16_t>{0, 0, 1, 0, 0, 0, 1, 1}),
            cngn::Column(std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"}),
            cngn::Column(std::vector<int32_t>{14, 22, 8, 88, 69, 67, 0, 1}),
        },
        schema);

    const std::string filename = "filter.chsv";
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    auto filter = std::make_unique<cngn::operators::Filter>(
        std::make_unique<cngn::operators::Scan>(filename, cngn::Schema({
                                                              {"b", cngn::Type::Int16},
                                                              {"c", cngn::Type::String},
                                                              {"d", cngn::Type::Int32},
                                                          })),
        std::make_shared<cngn::operators::BinaryExpression>(
            cngn::operators::BinaryExpressionType::Neq,
            std::make_shared<cngn::operators::SelectExpression>("b"),
            std::make_shared<cngn::operators::ConstantExpression>(static_cast<int16_t>(0))));

    filter->Open();

    auto ans_op = filter->Next();

    filter->Close();

    REQUIRE(ans_op.has_value());

    auto ans = ans_op.value();

    REQUIRE(ans->RowCount() == 3);

    REQUIRE((*ans)[0] == cngn::Column(std::vector<int16_t>{1, 1, 1}));
    REQUIRE((*ans)[1] == cngn::Column(std::vector<std::string_view>{"c", "g", "h"}));
    REQUIRE((*ans)[2] == cngn::Column(std::vector<int32_t>{8, 0, 1}));
}