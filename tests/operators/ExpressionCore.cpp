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

TEST_CASE_METHOD(GlogFixture, "Simple div", "[Div expression]") {
    cngn::Column l(std::vector{14, 00, 88, -6, 32, -69, -8, -19});
    cngn::Column r(std::vector{2, 111, 88, 3, 17, -23, -3, 4});

    auto ans = cngn::operators::Div(l, r);

    REQUIRE(ans.Size() == l.Size());
    REQUIRE(ans.GetData() == cngn::ArrayTypeVariant(std::vector{7, 0, 1, -2, 1, 3, 2, -4}));
}

TEST_CASE_METHOD(GlogFixture, "Dividing by zero", "[Div expression]") {
    cngn::Column l(std::vector<int16_t>{14, 00, 88, -6, 32, -69, -8, -19});
    cngn::Column r(std::vector<int16_t>{0, 111, 88, 3, 17, -23, -3, 4});

    REQUIRE_THROWS(cngn::operators::Div(l, r));
}

TEST_CASE_METHOD(GlogFixture, "Dividing different types", "[Div expression]") {
    cngn::Column l(std::vector<__int128_t>{1000000000000000003ll, 00, 88, -6, 32, -69, -8, -19});
    cngn::Column r(std::vector{2, 111, 88, 3, 17, -23, -3, 4});

    auto ans = cngn::operators::Div(l, r);

    REQUIRE(ans.Size() == l.Size());
    REQUIRE(ans.GetData() == cngn::ArrayTypeVariant(std::vector<__int128_t>{500000000000000001ll, 0,
                                                                            1, -2, 1, 3, 2, -4}));
}

TEST_CASE_METHOD(GlogFixture, "Contains string_view match", "[Contains expression]") {
    char buffer[] = "foobar\0foobaz\0hello\0";
    cngn::Column col(std::vector{std::string_view(buffer, 6), std::string_view(buffer + 7, 6),
                                 std::string_view(buffer + 14, 5)});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(
        cngn::operators::Contains(col, "foo").GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 1);
    REQUIRE(ans[2] == 0);
}

TEST_CASE_METHOD(GlogFixture, "Contains metastring match", "[Contains expression]") {
    cngn::Column col(std::vector<std::string>{"hello world", "foobar", "baz"});

    auto ans =
        std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Contains(col, "oo").GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 0);
    REQUIRE(ans[1] == 1);
    REQUIRE(ans[2] == 0);
}

TEST_CASE_METHOD(GlogFixture, "Contains empty substr", "[Contains expression]") {
    cngn::Column col(std::vector<std::string>{"abc", "", "xyz"});

    auto ans =
        std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Contains(col, "").GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 1);
    REQUIRE(ans[2] == 1);
}

TEST_CASE_METHOD(GlogFixture, "Contains wrong type throws", "[Contains expression]") {
    cngn::Column col(std::vector<int64_t>{1, 2, 3});

    REQUIRE_THROWS(cngn::operators::Contains(col, "x"));
}

TEST_CASE_METHOD(GlogFixture, "And elementwise", "[And expression]") {
    cngn::Column l(std::vector<char>{1, 0, 1});
    cngn::Column r(std::vector<char>{1, 1, 0});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::And(l, r).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 0);
    REQUIRE(ans[2] == 0);
}

TEST_CASE_METHOD(GlogFixture, "And all true", "[And expression]") {
    cngn::Column l(std::vector<char>{1, 1, 1});
    cngn::Column r(std::vector<char>{1, 1, 1});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::And(l, r).GetData());

    REQUIRE(ans.size() == 3);
    for (size_t i = 0; i < ans.size(); i++) {
        REQUIRE(ans[i] == 1);
    }
}

TEST_CASE_METHOD(GlogFixture, "And all false", "[And expression]") {
    cngn::Column l(std::vector<char>{0, 0, 0});
    cngn::Column r(std::vector<char>{1, 0, 1});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::And(l, r).GetData());

    REQUIRE(ans.size() == 3);
    for (size_t i = 0; i < ans.size(); i++) {
        REQUIRE(ans[i] == 0);
    }
}

TEST_CASE_METHOD(GlogFixture, "And non-bool throws", "[And expression]") {
    cngn::Column l(std::vector<int64_t>{1, 0, 1});
    cngn::Column r(std::vector<int64_t>{1, 1, 0});

    REQUIRE_THROWS(cngn::operators::And(l, r));
}

TEST_CASE_METHOD(GlogFixture, "Simple min_max", "[MinMax expression]") {
    cngn::Column integer(std::vector{14, 00, 88, -6, 32, -69, -8, -19});
    cngn::Column str(std::vector<std::string_view>{"aboba", "aacb", "aabc", "aacba"});

    auto ans_str = cngn::operators::Min(str);
    auto ans_int = cngn::operators::Max(integer);

    REQUIRE(ans_int.has_value());
    REQUIRE(ans_str.has_value());

    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::Int32>>(ans_int.value()) == 88);
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::String>>(ans_str.value()) == "aabc");
}
