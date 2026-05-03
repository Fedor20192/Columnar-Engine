#include "../Fixtures.h"
#include "ExpressionsCore.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Simple Neq", "[Neq Expression]") {
    cngn::Column l(std::vector{14, 00, 88});
    cngn::Column r(std::vector{14, 00, 87});

    auto ans =
        std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::NotEqual(l, r).GetData());

    REQUIRE(ans.size() == l.Size());
    REQUIRE(ans[0] == 0);
    REQUIRE(ans[1] == 0);
    REQUIRE(ans[2] == 1);
}

TEST_CASE_METHOD(GlogFixture, "String Neq", "[Neq Expression]") {
    char buffer_1[] = "abobabebra\0";
    char buffer_2[] = "bebra\0";
    cngn::Column l(std::vector{std::string_view(buffer_1, buffer_1 + 5),
                               std::string_view(buffer_1 + 5, buffer_1 + 10)});
    cngn::Column r(std::vector{std::string_view(buffer_2, buffer_2 + 5),
                               std::string_view(buffer_2, buffer_2 + 5)});

    auto ans =
        std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::NotEqual(l, r).GetData());

    REQUIRE(ans.size() == l.Size());
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 0);
}

TEST_CASE_METHOD(GlogFixture, "Different types", "[Neq Expression]") {
    cngn::Column l(std::vector{14, 00, 88});
    cngn::Column r(std::vector<cngn::PhysicalType<cngn::Type::UInt64>>{14, 00, 87});

    REQUIRE_THROWS(cngn::operators::NotEqual(l, r).GetData());
}