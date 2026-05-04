#include "Aggregation.h"

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Simple sum aggregation", "[Aggregation operator]") {
    auto batch = DefaultTestConfig::DefaultPrepare();

    auto scan = std::make_unique<cngn::operators::Scan>(
            DefaultTestConfig::kFilename,
            cngn::Schema({
                {"a", cngn::Type::Int64},
                {"b", cngn::Type::Int64},
                {"name123", cngn::Type::String},
                {"d", cngn::Type::Int64},
            }));

    auto sum = std::make_unique<cngn::operators::Aggregation>(
        std::move(scan),
        std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("a"), "sum_a"},
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("b"), "sum_b"},
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("d"), "sum_d"},

        }
    );

    sum->Open();

    auto ans = sum->Next().value_or(nullptr);

    REQUIRE(ans);

    REQUIRE(ans->ColumnCount() == 3);
    REQUIRE(ans->RowCount() == 1);

    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int128>>((*ans)[0][0]) == static_cast<__int128_t>(14));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int128>>((*ans)[1][0]) == static_cast<__int128_t>(16));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int128>>((*ans)[2][0]) == static_cast<__int128_t>(8));

    sum->Close();
}

TEST_CASE_METHOD(GlogFixture, "Strings sum", "[Aggregation operator]") {
    auto batch = DefaultTestConfig::DefaultPrepare();

    auto scan = std::make_unique<cngn::operators::Scan>(
            DefaultTestConfig::kFilename,
            cngn::Schema({
                {"a", cngn::Type::Int64},
                {"b", cngn::Type::Int64},
                {"name123", cngn::Type::String},
                {"d", cngn::Type::Int64},
            }));

    auto sum = std::make_unique<cngn::operators::Aggregation>(
        std::move(scan),
        std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("a"), "sum_a"},
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("name123"), "sum_name"},
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("d"), "sum_d"},
        }
    );

    sum->Open();

    REQUIRE_THROWS(sum->Next());

    sum->Close();
}