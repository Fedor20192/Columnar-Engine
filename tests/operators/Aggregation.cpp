#include "Aggregation.h"

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

#include <unordered_map>

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

    REQUIRE_FALSE(sum->Next());

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


TEST_CASE_METHOD(GlogFixture, "Simple distinct aggregation", "[Aggregation operator]") {
    cngn::Schema schema(std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int32},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::UInt64}
    });

    cngn::Batch batch_1(std::vector{
        cngn::Column{std::vector<int64_t>{1, 2, 3, 4, 5, 4, 2, 1}},
        cngn::Column{std::vector<int32_t>{1, 1, 1, 1, 1, 1, 1, 1}},
        cngn::Column{std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"}},
        cngn::Column{std::vector<uint64_t>{111, 222, 333, 444, 555, 666, 777, 888}}
    }, schema
    );

    cngn::Batch batch_2(std::vector{
        cngn::Column{std::vector<int64_t>{11, 22, 33, 44, 55, 44, 22, 11}},
        cngn::Column{std::vector<int32_t>{1, 1, 1, 1, 1, 1, 1, 1}},
        cngn::Column{std::vector<std::string>{"a", "boba", "c", "ement", "e", "balo", "g", "opa"}},
        cngn::Column{std::vector<uint64_t>{999, 22, 33, 44, 55, 66, 77, 88}}
    }, schema
    );

    cngn::BatchedWriter writer(DefaultTestConfig::kFilename, schema);
    writer.WriteBatch(batch_1);
    writer.WriteBatch(batch_2);
    writer.WriteMetadata();
    writer.Flush();


    auto scan = std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, schema);

    auto sum = std::make_unique<cngn::operators::Aggregation>(
        std::move(scan),
        std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("a"), "distinct_a"},
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("b"), "distinct_b"},
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("name123"), "distinct_name"},
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("d"), "distinct_d"},
        }
    );

    sum->Open();

    auto ans = sum->Next().value_or(nullptr);

    REQUIRE(ans);

    REQUIRE(ans->ColumnCount() == 4);
    REQUIRE(ans->RowCount() == 1);

    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>((*ans)[0][0]) == static_cast<uint64_t>(10));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>((*ans)[1][0]) == static_cast<uint64_t>(1));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>((*ans)[2][0]) == static_cast<uint64_t>(12));
    REQUIRE(std::get<cngn::PhysicalType<cngn::Type::UInt64>>((*ans)[3][0]) == static_cast<uint64_t>(16));

    REQUIRE_FALSE(sum->Next());

    sum->Close();
}

TEST_CASE_METHOD(GlogFixture, "Simple group by aggregation", "[Aggregation operator]") {
    cngn::Schema schema(std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int32},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::UInt64}
    });

    cngn::Batch batch_1(std::vector{
        cngn::Column{std::vector<int64_t>{1, 2, 3, 4, 5, 4, 2, 1}},
        cngn::Column{std::vector<int32_t>{132, 1467, 1232, 4, 545, 789, 12, 13}},
        cngn::Column{std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"}},
        cngn::Column{std::vector<uint64_t>{111, 222, 333, 444, 555, 666, 777, 888}}
    }, schema
    );

    cngn::Batch batch_2(std::vector{
        cngn::Column{std::vector<int64_t>{3, 2, 3, 5, 55, 1, 1, 4}},
        cngn::Column{std::vector<int32_t>{314, 358, 6, -234, 320, 809, -69, 13}},
        cngn::Column{std::vector<std::string>{"a", "boba", "c", "ement", "e", "balo", "g", "opa"}},
        cngn::Column{std::vector<uint64_t>{999, 22, 33, 44, 55, 66, 77, 88}}
    }, schema
    );

    cngn::BatchedWriter writer(DefaultTestConfig::kFilename, schema);
    writer.WriteBatch(batch_1);
    writer.WriteBatch(batch_2);
    writer.WriteMetadata();
    writer.Flush();


    auto scan = std::make_unique<cngn::operators::Scan>(DefaultTestConfig::kFilename, schema);

    auto sum = std::make_unique<cngn::operators::Aggregation>(
        std::move(scan),
        std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Sum, std::make_shared<cngn::operators::SelectExpression>("a"), "distinct_a"},
            {cngn::operators::AggregationType::Max, std::make_shared<cngn::operators::SelectExpression>("b"), "distinct_b"},
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("d"), "distinct_d"},
        },
        std::vector<cngn::operators::GroupByMeta>{
            {std::make_shared<cngn::operators::SelectExpression>("name123"), "group_name"},
        }
    );

    sum->Open();

    auto ans = sum->Next().value_or(nullptr);

    REQUIRE(ans);

    REQUIRE(ans->ColumnCount() == 4);
    REQUIRE(ans->RowCount() == 12);

    std::unordered_map<std::string, size_t> row_by_name;
    for (size_t i = 0; i < ans->RowCount(); ++i) {
        row_by_name[std::string(std::get<std::string_view>((*ans)[0][i]))] = i;
    }

    auto check = [&](const std::string& name, __int128_t sum_a, int32_t max_b, uint64_t distinct_d) {
        const size_t i = row_by_name.at(name);
        REQUIRE(std::get<__int128_t>((*ans)[1][i]) == sum_a);
        REQUIRE(std::get<int32_t>((*ans)[2][i]) == max_b);
        REQUIRE(std::get<uint64_t>((*ans)[3][i]) == distinct_d);
    };

    check("a",     4,    314,   2);
    check("b",     2,   1467,   1);
    check("boba",  2,    358,   1);
    check("c",     6,   1232,   2);
    check("d",     4,      4,   1);
    check("e",    60,    545,   2);
    check("ement", 5,   -234,   1);
    check("f",     4,    789,   1);
    check("balo",  1,    809,   1);
    check("g",     3,     12,   2);
    check("h",     1,     13,   1);
    check("opa",   4,     13,   1);

    REQUIRE_FALSE(sum->Next());

    sum->Close();
}
