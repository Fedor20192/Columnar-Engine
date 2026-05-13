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

TEST_CASE_METHOD(GlogFixture, "ContainsExpression on string column", "[Contains expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("name123");
    auto contains = std::make_shared<cngn::operators::ContainsExpression>(col, "ir");

    auto ans = contains->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[0]) == 1);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[1]) == 0);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[2]) == 1);
}

TEST_CASE_METHOD(GlogFixture, "ContainsExpression no match", "[Contains expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("name123");
    auto contains = std::make_shared<cngn::operators::ContainsExpression>(col, "xyz");

    auto ans = contains->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    for (size_t i = 0; i < ans.Size(); i++) {
        REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[i]) == 0);
    }
}

TEST_CASE_METHOD(GlogFixture, "ContainsExpression on non-string throws", "[Contains expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("a");
    auto contains = std::make_shared<cngn::operators::ContainsExpression>(col, "1");

    REQUIRE_THROWS(contains->Calculate(batch));
}

TEST_CASE_METHOD(GlogFixture, "And Expression two contains", "[And expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("name123");
    auto contains_ir = std::make_shared<cngn::operators::ContainsExpression>(col, "ir");
    auto contains_irst = std::make_shared<cngn::operators::ContainsExpression>(col, "irst");

    auto and_expr = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::And, contains_ir, contains_irst);

    auto ans = and_expr->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[0]) == 1);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[1]) == 0);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[2]) == 0);
}

TEST_CASE_METHOD(GlogFixture, "And Expression contains and neq", "[And expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto contains_ir = std::make_shared<cngn::operators::ContainsExpression>(
        std::make_shared<cngn::operators::SelectExpression>("name123"), "ir");
    auto neq_1 = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::Neq,
        std::make_shared<cngn::operators::SelectExpression>("a"),
        std::make_shared<cngn::operators::ConstantExpression>(static_cast<int64_t>(1)));

    auto and_expr = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::And, contains_ir, neq_1);

    auto ans = and_expr->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[0]) == 0);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[1]) == 0);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[2]) == 1);
}

TEST_CASE_METHOD(GlogFixture, "Gt Expression column vs constant", "[Binary expressions]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("a");
    auto constant = std::make_shared<cngn::operators::ConstantExpression>(static_cast<int64_t>(4));

    auto gt = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::Gt, col, constant);

    auto ans = gt->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[0]) == 0);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[1]) == 1);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[2]) == 1);
}

TEST_CASE_METHOD(GlogFixture, "Gt Expression two constants equal", "[Binary expressions]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto lhs = std::make_shared<cngn::operators::ConstantExpression>(static_cast<int64_t>(5));
    auto rhs = std::make_shared<cngn::operators::ConstantExpression>(static_cast<int64_t>(5));

    auto gt = std::make_shared<cngn::operators::BinaryExpression>(
        cngn::operators::BinaryExpressionType::Gt, lhs, rhs);

    auto ans = gt->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    for (size_t i = 0; i < ans.Size(); i++) {
        REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Bool>>(ans[i]) == 0);
    }
}

TEST_CASE_METHOD(GlogFixture, "StrLenExpression on string column", "[StrLen expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("name123");
    auto strlen_expr = std::make_shared<cngn::operators::StrLenExpression>(col);

    auto ans = strlen_expr->Calculate(batch);

    REQUIRE(ans.Size() == batch->RowCount());
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>(ans[0]) == 5);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>(ans[1]) == 6);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>(ans[2]) == 5);
}

TEST_CASE_METHOD(GlogFixture, "StrLenExpression on non-string throws", "[StrLen expression]") {
    auto batch = std::make_shared<cngn::Batch>(DefaultTestConfig::DefaultPrepare());

    auto col = std::make_shared<cngn::operators::SelectExpression>("a");
    auto strlen_expr = std::make_shared<cngn::operators::StrLenExpression>(col);

    REQUIRE_THROWS(strlen_expr->Calculate(batch));
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