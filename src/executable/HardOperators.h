#pragma once

#include <functional>

#include "Aggregation.h"
#include "Count.h"
#include "Expression.h"
#include "Filter.h"
#include "Gluing.h"
#include "Projector.h"
#include "Scan.h"
#include "Sort.h"

using QueryGenerator =
    std::function<std::unique_ptr<cngn::operators::Operator>(const std::string&)>;

using Aggregation = cngn::operators::Aggregation;
using AggregationMeta = cngn::operators::AggregationMeta;
using AggregationType = cngn::operators::AggregationType;
using BinaryExpression = cngn::operators::BinaryExpression;
using BinaryExpressionType = cngn::operators::BinaryExpressionType;
using ConstantExpression = cngn::operators::ConstantExpression;
using Count = cngn::operators::Count;
using Filter = cngn::operators::Filter;
using Gluing = cngn::operators::Gluing;
using GroupByMeta = cngn::operators::GroupByMeta;
using Operator = cngn::operators::Operator;
using Projector = cngn::operators::Projector;
using ProjectionMeta = cngn::operators::ProjectionMeta;
using Scan = cngn::operators::Scan;
using Schema = cngn::Schema;
using SelectExpression = cngn::operators::SelectExpression;
using Sort = cngn::operators::Sort;
using SortKey = cngn::operators::SortKey;
using Type = cngn::Type;
using TopK = cngn::operators::TopK;

constexpr int kQueriesCount = 13;

const std::array<QueryGenerator, kQueriesCount> kGenerators = {
    [](const std::string& filename) {
        return std::make_unique<Count>(std::make_unique<Scan>(filename, Schema()));
    },
    [](const std::string& filename) {
        auto filter = std::make_unique<Count>(std::make_unique<Filter>(
            std::make_unique<Scan>(filename, Schema({{"AdvEngineID", Type::Int16}})),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_unique<SelectExpression>("AdvEngineID"),
                std::make_unique<ConstantExpression>(static_cast<int16_t>(0)))));

        return filter;
    },
    [](const std::string& filename) {
        auto scan_1 = std::make_unique<Scan>(
            filename, Schema({{"AdvEngineID", Type::Int16}, {"ResolutionWidth", Type::Int16}}));

        auto agg = std::make_unique<Aggregation>(
            std::move(scan_1),
            std::vector<AggregationMeta>{
                {AggregationType::Sum, std::make_shared<SelectExpression>("AdvEngineID"), "sum_id"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("ResolutionWidth"),
                 "sum_width"},
            });

        auto count = std::make_unique<Count>(std::make_unique<Scan>(filename, Schema()), "count");

        std::vector<std::unique_ptr<Operator>> aggs;
        aggs.push_back(std::move(agg));
        aggs.push_back(std::move(count));

        return std::make_unique<Projector>(
            std::make_unique<Gluing>(std::move(aggs)),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("sum_id"), "sum"},
                {std::make_shared<SelectExpression>("count"), "count"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Div,
                                                    std::make_shared<SelectExpression>("sum_width"),
                                                    std::make_shared<SelectExpression>("count")),
                 "avg"},
            });
    },
    [](const std::string& filename) {
        auto scan_1 = std::make_unique<Scan>(filename, Schema({{"UserID", Type::Int64}}));

        auto agg = std::make_unique<Aggregation>(
            std::move(scan_1),
            std::vector<AggregationMeta>{
                {AggregationType::Sum, std::make_shared<SelectExpression>("UserID"), "sum_id"}});

        auto count = std::make_unique<Count>(std::make_unique<Scan>(filename, Schema()), "count");

        std::vector<std::unique_ptr<Operator>> aggs;
        aggs.push_back(std::move(agg));
        aggs.push_back(std::move(count));

        return std::make_unique<Projector>(
            std::make_unique<Gluing>(std::move(aggs)),
            std::vector<ProjectionMeta>{
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Div,
                                                    std::make_shared<SelectExpression>("sum_id"),
                                                    std::make_shared<SelectExpression>("count")),
                 "avg"},
            });
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"UserID", Type::Int64}}));

        return std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{{AggregationType::Distinct,
                                          std::make_shared<SelectExpression>("UserID"), "count"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"SearchPhrase", Type::String}}));

        return std::make_unique<Aggregation>(
            std::move(scan), std::vector<AggregationMeta>{
                                 {AggregationType::Distinct,
                                  std::make_shared<SelectExpression>("SearchPhrase"), "count"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"Eventdate", Type::String}}));

        return std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Min, std::make_shared<SelectExpression>("Eventdate"), "min"},
                {AggregationType::Max, std::make_shared<SelectExpression>("Eventdate"), "max"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"AdvEngineID", Type::Int16}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("AdvEngineID"),
                std::make_shared<ConstantExpression>(static_cast<int16_t>(0))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("AdvEngineID"), "AdvEngineID"}});

        return std::make_unique<Sort>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "AdvEngineID"}},
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"RegionID", Type::Int32}, {"UserID", Type::Int64}}));

        auto aggr = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Distinct, std::make_shared<SelectExpression>("UserID"), "count"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("RegionID"), "RegionID"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "RegionID"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"RegionID", Type::Int32},
                                                             {"AdvEngineID", Type::Int16},
                                                             {"ResolutionWidth", Type::Int16},
                                                             {"UserID", Type::Int64}}));

        auto aggr = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Sum, std::make_shared<SelectExpression>("AdvEngineID"),
                 "sum_adv"},
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("ResolutionWidth"),
                 "sum_res"},
                {AggregationType::Distinct, std::make_shared<SelectExpression>("UserID"),
                 "distinct_users"},
            },
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("RegionID"), "RegionID"}});

        auto proj = std::make_unique<Projector>(
            std::move(aggr),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("RegionID"), "RegionID"},
                {std::make_shared<SelectExpression>("sum_adv"), "SUM(AdvEngineID)"},
                {std::make_shared<SelectExpression>("c"), "c"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Div,
                                                    std::make_shared<SelectExpression>("sum_res"),
                                                    std::make_shared<SelectExpression>("c")),
                 "AVG(ResolutionWidth)"},
                {std::make_shared<SelectExpression>("distinct_users"), "COUNT(DISTINCT UserID)"},
            });

        return std::make_unique<TopK>(
            std::move(proj), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"MobilePhoneModel", Type::String}, {"UserID", Type::Int64}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_unique<SelectExpression>("MobilePhoneModel"),
                std::make_unique<ConstantExpression>(std::string_view(""))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Distinct, std::make_unique<SelectExpression>("UserID"), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("MobilePhoneModel"), "MobilePhoneModel"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "MobilePhoneModel"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"MobilePhone", Type::Int16},
                                                             {"MobilePhoneModel", Type::String},
                                                             {"UserID", Type::Int64}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_unique<SelectExpression>("MobilePhoneModel"),
                std::make_unique<ConstantExpression>(std::string_view(""))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Distinct, std::make_unique<SelectExpression>("UserID"), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("MobilePhone"), "MobilePhone"},
                {std::make_shared<SelectExpression>("MobilePhoneModel"), "MobilePhoneModel"},
            });

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "MobilePhoneModel"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"SearchPhrase", Type::String}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_unique<SelectExpression>("SearchPhrase"),
                std::make_unique<ConstantExpression>(std::string_view(""))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_unique<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "SearchPhrase"}},
            10, false);
    }};
