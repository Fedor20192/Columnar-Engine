#include "Expression.h"

#include <memory>

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "catch2/catch_template_test_macros.hpp"

constexpr uint64_t kValue = 14;

TEST_CASE_METHOD(GlogFixture, "Constant Expression", "[Constant Expression]") {
    auto int_constant = std::make_shared<cngn::operators::ConstantExpression>(kValue);

    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto int_column = int_constant->Calculate(batch);

    REQUIRE(int_column.Size() == batch->RowCount());
    for (size_t i = 0; i < int_column.Size(); i++) {
        REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>(int_column[i]) == kValue);
    }

    const std::string string = "string";
    auto string_constant = std::make_shared<cngn::operators::ConstantExpression>(string);

    auto string_column = string_constant->Calculate(batch);

    REQUIRE(string_column.Size() == batch->RowCount());
    for (size_t i = 0; i < int_column.Size(); i++) {
        REQUIRE(std::get<cngn::PhysicalType<cngn::Type::MetaString>>(string_column[i]) == string);
    }
}

TEST_CASE_METHOD(GlogFixture, "Neq Constants Expression", "[Binary expressions]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto int_constant_1 =
        std::make_shared<cngn::operators::ConstantExpression>(std::string("aboba"));
    auto int_constant_2 =
        std::make_shared<cngn::operators::ConstantExpression>(std::string("bebra"));

    auto neq = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::Neq, int_constant_1, int_constant_2);

    auto ans = neq->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    for (size_t i = 0; i < ans.Size(); i++) {
        REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[i]) == 1);
    }
}

TEST_CASE_METHOD(GlogFixture, "Neq Expression", "[Binary expressions]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto int_constant =
        std::make_shared<cngn::operators::ConstantExpression>(static_cast<int64_t>(5));
    auto int_column = std::make_shared<cngn::operators::SelectExpression>("a");

    auto neq = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::Neq, int_constant, int_column);

    auto ans = neq->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[0]) == 1);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[1]) == 0);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[2]) == 1);
}