#pragma once

#include <functional>

#include "Aggregation.h"
#include "Count.h"
#include "Expression.h"
#include "Filter.h"
#include "Gluing.h"
#include "Projector.h"
#include "Scan.h"

using QueryGenerator =
    std::function<std::unique_ptr<cngn::operators::Operator>(const std::string&)>;

constexpr int kQueriesCount = 7;

const std::array<QueryGenerator, kQueriesCount> kGenerators = {
    [](const std::string& filename) {
        return std::make_unique<cngn::operators::Count>(
            std::make_unique<cngn::operators::Scan>(filename, cngn::Schema()));
    },
    [](const std::string& filename) {
        auto filter =
            std::make_unique<cngn::operators::Count>(std::make_unique<cngn::operators::Filter>(
                std::make_unique<cngn::operators::Scan>(
                    filename, cngn::Schema({{"AdvEngineID", cngn::Type::Int16}})),
                std::make_shared<cngn::operators::BinaryExpression>(
                    cngn::operators::BinaryExpressionType::Neq,
                    std::make_unique<cngn::operators::SelectExpression>("AdvEngineID"),
                    std::make_unique<cngn::operators::ConstantExpression>(
                        static_cast<int16_t>(0)))));

        return filter;
    },
    [](const std::string& filename) {
        auto scan_1 = std::make_unique<cngn::operators::Scan>(
            filename, cngn::Schema({{"AdvEngineID", cngn::Type::Int16},
                                    {"ResolutionWidth", cngn::Type::Int16}}));

        auto agg = std::make_unique<cngn::operators::Aggregation>(
            std::move(scan_1),
            std::vector<cngn::operators::AggregationMeta>{
                {cngn::operators::AggregationType::Sum,
                 std::make_shared<cngn::operators::SelectExpression>("AdvEngineID"), "sum_id"},
                {cngn::operators::AggregationType::Sum,
                 std::make_shared<cngn::operators::SelectExpression>("ResolutionWidth"),
                 "sum_width"},
            });

        auto count = std::make_unique<cngn::operators::Count>(
            std::make_unique<cngn::operators::Scan>(filename, cngn::Schema()), "count");

        std::vector<std::unique_ptr<cngn::operators::Operator>> aggs;
        aggs.push_back(std::move(agg));
        aggs.push_back(std::move(count));

        return std::make_unique<cngn::operators::Projector>(
            std::make_unique<cngn::operators::Gluing>(std::move(aggs)),
            std::vector<cngn::operators::ProjectionMeta>{
                {std::make_shared<cngn::operators::SelectExpression>("sum_id"), "sum"},
                {std::make_shared<cngn::operators::SelectExpression>("count"), "count"},
                {std::make_shared<cngn::operators::BinaryExpression>(
                     cngn::operators::BinaryExpressionType::Div,
                     std::make_shared<cngn::operators::SelectExpression>("sum_width"),
                     std::make_shared<cngn::operators::SelectExpression>("count")),
                 "avg"},
            });
    },
    [](const std::string& filename) {
        auto scan_1 = std::make_unique<cngn::operators::Scan>(
            filename, cngn::Schema({{"UserID", cngn::Type::Int64}}));

        auto agg = std::make_unique<cngn::operators::Aggregation>(
            std::move(scan_1),
            std::vector<cngn::operators::AggregationMeta>{
                {cngn::operators::AggregationType::Sum,
                 std::make_shared<cngn::operators::SelectExpression>("UserID"), "sum_id"}});

        auto count = std::make_unique<cngn::operators::Count>(
            std::make_unique<cngn::operators::Scan>(filename, cngn::Schema()), "count");

        std::vector<std::unique_ptr<cngn::operators::Operator>> aggs;
        aggs.push_back(std::move(agg));
        aggs.push_back(std::move(count));

        return std::make_unique<cngn::operators::Projector>(
            std::make_unique<cngn::operators::Gluing>(std::move(aggs)),
            std::vector<cngn::operators::ProjectionMeta>{
                {std::make_shared<cngn::operators::BinaryExpression>(
                     cngn::operators::BinaryExpressionType::Div,
                     std::make_shared<cngn::operators::SelectExpression>("sum_id"),
                     std::make_shared<cngn::operators::SelectExpression>("count")),
                 "avg"},
            });
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<cngn::operators::Scan>(filename, cngn::Schema({{"UserID", cngn::Type::Int64}}));

        return std::make_unique<cngn::operators::Aggregation>(std::move(scan), std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("UserID"), "count" }
        });
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<cngn::operators::Scan>(filename, cngn::Schema({{"SearchPhrase", cngn::Type::String}}));

        return std::make_unique<cngn::operators::Aggregation>(std::move(scan), std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Distinct, std::make_shared<cngn::operators::SelectExpression>("SearchPhrase"), "count" }
        });
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<cngn::operators::Scan>(filename, cngn::Schema({{"Eventdate", cngn::Type::String}}));

        return std::make_unique<cngn::operators::Aggregation>(std::move(scan), std::vector<cngn::operators::AggregationMeta>{
            {cngn::operators::AggregationType::Min, std::make_shared<cngn::operators::SelectExpression>("Eventdate"), "min" },
            {cngn::operators::AggregationType::Max, std::make_shared<cngn::operators::SelectExpression>("Eventdate"), "max" }
        });
    }
};
