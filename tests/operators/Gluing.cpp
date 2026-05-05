#include "Gluing.h"

#include <catch2/catch_template_test_macros.hpp>

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "Aggregation.h"
#include "Scan.h"

TEST_CASE_METHOD(GlogFixture, "Simple gluing", "[Gluing Operator]") {
    auto batch = DefaultTestConfig::DefaultPrepare();

    auto scan_1 = std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename,
                                                          cngn::Schema({
                                                              {"b", cngn::Type::Int64},
                                                              {"name123", cngn::Type::String},
                                                          }));

    auto sum_1 = std::make_unique<cngn::operators::Aggregation>(
        std::move(scan_1), std::vector<cngn::operators::AggregationMeta>{
                               {cngn::operators::AggregationType::Sum,
                                std::make_shared<cngn::operators::SelectExpression>("b"), "sum_b"},

                           });

    auto scan_2 = std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename,
                                                          cngn::Schema({
                                                              {"d", cngn::Type::Int64},
                                                              {"a", cngn::Type::Int64},
                                                          }));

    auto sum_2 = std::make_unique<cngn::operators::Aggregation>(
        std::move(scan_2), std::vector<cngn::operators::AggregationMeta>{
                               {cngn::operators::AggregationType::Sum,
                                std::make_shared<cngn::operators::SelectExpression>("a"), "sum_b"},
                               {cngn::operators::AggregationType::Sum,
                                std::make_shared<cngn::operators::SelectExpression>("d"), "sum_d"},

                           });

    auto vec = std::vector<std::unique_ptr<cngn::operators::Operator>>();
    vec.push_back(std::move(sum_1));
    vec.push_back(std::move(sum_2));

    auto glue = std::make_unique<cngn::operators::Gluing>(std::move(vec));

    glue->Open();

    auto ans = glue->Next().value_or(nullptr);

    REQUIRE(ans);

    REQUIRE(ans->ColumnCount() == 3);
    REQUIRE(ans->RowCount() == 1);

    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int64>>((*ans)[0][0]) ==
            static_cast<int64_t>(16));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int64>>((*ans)[1][0]) ==
            static_cast<int64_t>(14));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int64>>((*ans)[2][0]) ==
            static_cast<int64_t>(8));

    REQUIRE_FALSE(glue->Next());

    glue->Close();
}