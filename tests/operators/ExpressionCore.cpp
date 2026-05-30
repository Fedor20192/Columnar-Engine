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
        cngn::operators::Contains(col, "foo", false).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 1);
    REQUIRE(ans[2] == 0);
}

TEST_CASE_METHOD(GlogFixture, "Contains metastring match", "[Contains expression]") {
    cngn::Column col(std::vector<std::string>{"hello world", "foobar", "baz"});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(
        cngn::operators::Contains(col, "oo", true).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 0);
    REQUIRE(ans[2] == 1);
}

TEST_CASE_METHOD(GlogFixture, "Contains empty substr", "[Contains expression]") {
    cngn::Column col(std::vector<std::string>{"abc", "", "xyz"});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(
        cngn::operators::Contains(col, "", false).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 1);
    REQUIRE(ans[2] == 1);
}

TEST_CASE_METHOD(GlogFixture, "Contains wrong type throws", "[Contains expression]") {
    cngn::Column col(std::vector<int64_t>{1, 2, 3});

    REQUIRE_THROWS(cngn::operators::Contains(col, "x", false));
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

TEST_CASE_METHOD(GlogFixture, "Gt basic", "[Gt expression]") {
    cngn::Column l(std::vector{3, 5, 1, 7});
    cngn::Column r(std::vector{2, 5, 4, 0});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Gt(l, r).GetData());

    REQUIRE(ans.size() == 4);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 0);
    REQUIRE(ans[2] == 0);
    REQUIRE(ans[3] == 1);
}

TEST_CASE_METHOD(GlogFixture, "Gt all false", "[Gt expression]") {
    cngn::Column l(std::vector<int64_t>{1, 2, 3});
    cngn::Column r(std::vector<int64_t>{4, 5, 6});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Gt(l, r).GetData());

    REQUIRE(ans.size() == 3);
    for (size_t i = 0; i < ans.size(); i++) {
        REQUIRE(ans[i] == 0);
    }
}

TEST_CASE_METHOD(GlogFixture, "Gt different types throws", "[Gt expression]") {
    cngn::Column l(std::vector{1, 2, 3});
    cngn::Column r(std::vector<int64_t>{4, 5, 6});

    REQUIRE_THROWS(cngn::operators::Gt(l, r));
}

TEST_CASE_METHOD(GlogFixture, "StrLen string_view", "[StrLen expression]") {
    char buffer[] = "hello\0world!\0hi\0";
    cngn::Column col(std::vector{std::string_view(buffer, 5), std::string_view(buffer + 6, 6),
                                 std::string_view(buffer + 13, 2)});

    auto ans =
        std::get<cngn::ArrayType<cngn::Type::UInt64>>(cngn::operators::StrLen(col).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 5);
    REQUIRE(ans[1] == 6);
    REQUIRE(ans[2] == 2);
}

TEST_CASE_METHOD(GlogFixture, "StrLen metastring", "[StrLen expression]") {
    cngn::Column col(std::vector<std::string>{"abc", "", "hello world"});

    auto ans =
        std::get<cngn::ArrayType<cngn::Type::UInt64>>(cngn::operators::StrLen(col).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 3);
    REQUIRE(ans[1] == 0);
    REQUIRE(ans[2] == 11);
}

TEST_CASE_METHOD(GlogFixture, "StrLen wrong type throws", "[StrLen expression]") {
    cngn::Column col(std::vector<int64_t>{1, 2, 3});

    REQUIRE_THROWS(cngn::operators::StrLen(col));
}

TEST_CASE_METHOD(GlogFixture, "Regex no match", "[Regex expression]") {
    cngn::Column col(std::vector<std::string>{"foo", "bar"});

    auto result = cngn::operators::Regex(col, re2::RE2("xyz"));
    const auto& ans = std::get<cngn::ArrayType<cngn::Type::String>>(result.GetData());

    REQUIRE(ans.size() == 2);
    REQUIRE(ans[0].empty());
    REQUIRE(ans[1].empty());
}

TEST_CASE_METHOD(GlogFixture, "Regex wrong type throws", "[Regex expression]") {
    cngn::Column col(std::vector<int64_t>{1, 2, 3});

    REQUIRE_THROWS(cngn::operators::Regex(col, re2::RE2(".")));
}

TEST_CASE_METHOD(GlogFixture, "Add basic", "[Add expression]") {
    cngn::Column l(std::vector{1, 2, 3, -5});
    cngn::Column r(std::vector{4, -1, 0, 5});

    auto ans = cngn::operators::Add(l, r);

    REQUIRE(ans.Size() == l.Size());
    REQUIRE(ans.GetData() == cngn::ArrayTypeVariant(std::vector{5, 1, 3, 0}));
}

TEST_CASE_METHOD(GlogFixture, "Add different types Int128 + Int32", "[Add expression]") {
    cngn::Column l(std::vector<__int128_t>{1000000000000000003ll, 0, -1});
    cngn::Column r(std::vector{2, 111, 1});

    auto ans = cngn::operators::Add(l, r);

    REQUIRE(ans.Size() == l.Size());
    REQUIRE(ans.GetData() ==
            cngn::ArrayTypeVariant(std::vector<__int128_t>{1000000000000000005ll, 111, 0}));
}

TEST_CASE_METHOD(GlogFixture, "Add non-arithmetic throws", "[Add expression]") {
    cngn::Column l(std::vector<std::string>{"a", "b"});
    cngn::Column r(std::vector<std::string>{"c", "d"});

    REQUIRE_THROWS(cngn::operators::Add(l, r));
}

TEST_CASE_METHOD(GlogFixture, "Mul basic", "[Mul expression]") {
    cngn::Column l(std::vector{2, 3, -4, 0});
    cngn::Column r(std::vector{5, -6, 7, 100});

    auto ans = cngn::operators::Mul(l, r);

    REQUIRE(ans.Size() == l.Size());
    REQUIRE(ans.GetData() == cngn::ArrayTypeVariant(std::vector{10, -18, -28, 0}));
}

TEST_CASE_METHOD(GlogFixture, "Mul different types UInt64 * Int32", "[Mul expression]") {
    cngn::Column l(std::vector<uint64_t>{100, 0, 7});
    cngn::Column r(std::vector{3, 999, 2});

    auto ans = cngn::operators::Mul(l, r);

    REQUIRE(ans.Size() == l.Size());
    REQUIRE(ans.GetData() == cngn::ArrayTypeVariant(std::vector<uint64_t>{300, 0, 14}));
}

TEST_CASE_METHOD(GlogFixture, "Mul non-arithmetic throws", "[Mul expression]") {
    cngn::Column l(std::vector<std::string>{"a", "b"});
    cngn::Column r(std::vector<std::string>{"c", "d"});

    REQUIRE_THROWS(cngn::operators::Mul(l, r));
}

TEST_CASE_METHOD(GlogFixture, "Or elementwise", "[Or expression]") {
    cngn::Column l(std::vector<char>{1, 0, 1});
    cngn::Column r(std::vector<char>{1, 1, 0});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Or(l, r).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 1);
    REQUIRE(ans[2] == 1);
}

TEST_CASE_METHOD(GlogFixture, "Or all true", "[Or expression]") {
    cngn::Column l(std::vector<char>{1, 1, 1});
    cngn::Column r(std::vector<char>{1, 0, 1});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Or(l, r).GetData());

    REQUIRE(ans.size() == 3);
    for (size_t i = 0; i < ans.size(); i++) {
        REQUIRE(ans[i] == 1);
    }
}

TEST_CASE_METHOD(GlogFixture, "Or all false", "[Or expression]") {
    cngn::Column l(std::vector<char>{0, 0, 0});
    cngn::Column r(std::vector<char>{0, 0, 0});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Bool>>(cngn::operators::Or(l, r).GetData());

    REQUIRE(ans.size() == 3);
    for (size_t i = 0; i < ans.size(); i++) {
        REQUIRE(ans[i] == 0);
    }
}

TEST_CASE_METHOD(GlogFixture, "Or non-bool throws", "[Or expression]") {
    cngn::Column l(std::vector<int64_t>{1, 0, 1});
    cngn::Column r(std::vector<int64_t>{1, 1, 0});

    REQUIRE_THROWS(cngn::operators::Or(l, r));
}

TEST_CASE_METHOD(GlogFixture, "Case mixed predicate", "[Case expression]") {
    cngn::Column pred(std::vector<char>{1, 0, 1});
    cngn::Column when_true(std::vector<int64_t>{10, 20, 30});
    cngn::Column when_false(std::vector<int64_t>{100, 200, 300});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Int64>>(
        cngn::operators::Case(pred, when_true, when_false).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 10);
    REQUIRE(ans[1] == 200);
    REQUIRE(ans[2] == 30);
}

TEST_CASE_METHOD(GlogFixture, "Case all true predicate", "[Case expression]") {
    cngn::Column pred(std::vector<char>{1, 1, 1});
    cngn::Column when_true(std::vector<int64_t>{1, 2, 3});
    cngn::Column when_false(std::vector<int64_t>{4, 5, 6});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Int64>>(
        cngn::operators::Case(pred, when_true, when_false).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 1);
    REQUIRE(ans[1] == 2);
    REQUIRE(ans[2] == 3);
}

TEST_CASE_METHOD(GlogFixture, "Case all false predicate", "[Case expression]") {
    cngn::Column pred(std::vector<char>{0, 0, 0});
    cngn::Column when_true(std::vector<int64_t>{1, 2, 3});
    cngn::Column when_false(std::vector<int64_t>{4, 5, 6});

    auto ans = std::get<cngn::ArrayType<cngn::Type::Int64>>(
        cngn::operators::Case(pred, when_true, when_false).GetData());

    REQUIRE(ans.size() == 3);
    REQUIRE(ans[0] == 4);
    REQUIRE(ans[1] == 5);
    REQUIRE(ans[2] == 6);
}

TEST_CASE_METHOD(GlogFixture, "Case type mismatch throws", "[Case expression]") {
    cngn::Column pred(std::vector<char>{1, 0, 1});
    cngn::Column when_true(std::vector<int64_t>{1, 2, 3});
    cngn::Column when_false(std::vector<int32_t>{4, 5, 6});

    REQUIRE_THROWS(cngn::operators::Case(pred, when_true, when_false));
}

TEST_CASE_METHOD(GlogFixture, "Case non-bool predicate throws", "[Case expression]") {
    cngn::Column pred(std::vector<int64_t>{1, 0, 1});
    cngn::Column when_true(std::vector<int64_t>{1, 2, 3});
    cngn::Column when_false(std::vector<int64_t>{4, 5, 6});

    REQUIRE_THROWS(cngn::operators::Case(pred, when_true, when_false));
}
