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
using CaseExpression = cngn::operators::CaseExpression;
using ConstantExpression = cngn::operators::ConstantExpression;
using ContainsExpression = cngn::operators::ContainsExpression;
using Count = cngn::operators::Count;
using ExtractMinute = cngn::operators::ExtractMinute;
using Filter = cngn::operators::Filter;
using Gluing = cngn::operators::Gluing;
using GroupByMeta = cngn::operators::GroupByMeta;
using Operator = cngn::operators::Operator;
using Projector = cngn::operators::Projector;
using ProjectionMeta = cngn::operators::ProjectionMeta;
using RegexExpression = cngn::operators::RegexExpression;
using Scan = cngn::operators::Scan;
using Schema = cngn::Schema;
using SelectExpression = cngn::operators::SelectExpression;
using Sort = cngn::operators::Sort;
using SortKey = cngn::operators::SortKey;
using StrLenExpression = cngn::operators::StrLenExpression;
using Type = cngn::Type;
using TopK = cngn::operators::TopK;

constexpr int kQueriesCount = 43;

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
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, false);
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
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
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
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
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
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
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
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"SearchPhrase", Type::String}, {"UserID", Type::Int64}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_unique<SelectExpression>("SearchPhrase"),
                std::make_unique<ConstantExpression>(std::string_view(""))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Distinct, std::make_unique<SelectExpression>("UserID"), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"SearchEngineID", Type::Int16}, {"SearchPhrase", Type::String}}));

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
                {std::make_shared<SelectExpression>("SearchEngineID"), "SearchEngineID"},
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"UserID", Type::Int64}}));

        auto aggr = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_unique<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("UserID"), "UserID"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"UserID", Type::Int64}, {"SearchPhrase", Type::String}}));

        auto aggr = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_unique<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("UserID"), "UserID"},
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"UserID", Type::Int64}, {"SearchPhrase", Type::String}}));

        auto aggr = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_unique<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("UserID"), "UserID"},
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("UserID"), "UserID"}}, 10,
            true);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"UserID", Type::Int64},
                                                             {"SearchPhrase", Type::String},
                                                             {"EventTime", Type::Timestamp}}));

        auto proj = std::make_unique<Projector>(
            std::move(scan),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("UserID"), "UserID"},
                {std::make_shared<ExtractMinute>(std::make_shared<SelectExpression>("EventTime")),
                 "EventTime"},
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        auto agg = std::make_unique<Aggregation>(
            std::move(proj),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("UserID"), "UserID"},
                {std::make_shared<SelectExpression>("EventTime"), "EventTime"},
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("count"), "count"}}, 10,
            false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"UserID", Type::Int64}}));

        return std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Eq, std::make_unique<SelectExpression>("UserID"),
                std::make_unique<ConstantExpression>(static_cast<int64_t>(435090932899640449))));
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"URL", Type::String}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan), std::make_shared<ContainsExpression>(
                                 std::make_shared<SelectExpression>("URL"), "google"));

        return std::make_unique<Count>(std::move(filter));
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"URL", Type::String}, {"SearchPhrase", Type::String}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<ContainsExpression>(std::make_shared<SelectExpression>("URL"),
                                                     "google"),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::Neq, std::make_shared<SelectExpression>("SearchPhrase"),
                    std::make_shared<ConstantExpression>(std::string_view("")))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Min, std::make_shared<SelectExpression>("URL"), "min_url"},
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
            },
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"URL", Type::String},
                                                             {"Title", Type::String},
                                                             {"SearchPhrase", Type::String},
                                                             {"UserID", Type::Int64}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<ContainsExpression>(std::make_shared<SelectExpression>("Title"),
                                                     "Google"),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<ContainsExpression>(std::make_shared<SelectExpression>("URL"),
                                                         ".google.", true),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Neq,
                        std::make_shared<SelectExpression>("SearchPhrase"),
                        std::make_shared<ConstantExpression>(std::string_view(""))))));

        auto aggr = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Min, std::make_shared<SelectExpression>("URL"), "min_url"},
                {AggregationType::Min, std::make_shared<SelectExpression>("Title"), "min_title"},
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
                {AggregationType::Distinct, std::make_shared<SelectExpression>("UserID"),
                 "distinct_users"},
            },
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});

        return std::make_unique<TopK>(
            std::move(aggr), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"URL", Type::String}, {"EventTime", Type::Timestamp}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan), std::make_shared<ContainsExpression>(
                                 std::make_shared<SelectExpression>("URL"), "google"));

        return std::make_unique<TopK>(
            std::move(filter),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("EventTime"), "EventTime"}},
            10, true);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"SearchPhrase", Type::String}, {"EventTime", Type::Timestamp}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("SearchPhrase"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        return std::make_unique<Projector>(
            std::make_unique<TopK>(
                std::move(filter),
                std::vector<SortKey>{
                    {std::make_shared<SelectExpression>("EventTime"), "EventTime"}},
                10, true),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"SearchPhrase", Type::String}, {"EventTime", Type::Timestamp}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("SearchPhrase"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        return std::make_unique<Projector>(
            std::make_unique<TopK>(
                std::move(filter),
                std::vector<SortKey>{
                    {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}},
                10, true),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"SearchPhrase", Type::String}, {"EventTime", Type::Timestamp}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("SearchPhrase"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        return std::make_unique<Projector>(
            std::make_unique<TopK>(
                std::move(filter),
                std::vector<SortKey>{
                    {std::make_shared<SelectExpression>("EventTime"), "EventTime"},
                    {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}},
                10, true),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("SearchPhrase"), "SearchPhrase"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(
            filename, Schema({{"CounterID", Type::Int32}, {"URL", Type::String}}));

        auto url_filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("URL"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        auto proj_1 = std::make_unique<Projector>(
            std::move(url_filter),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("CounterID"), "CounterID"},
                {std::make_shared<StrLenExpression>(std::make_shared<SelectExpression>("URL")),
                 "strlen"}});

        auto agg = std::make_unique<Aggregation>(
            std::move(proj_1),
            std::vector<AggregationMeta>{
                {AggregationType::Sum, std::make_shared<SelectExpression>("strlen"), "sum_strlen"},
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "count"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("CounterID"), "CounterID"}});

        auto count_filter = std::make_unique<Filter>(
            std::move(agg),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Gt, std::make_shared<SelectExpression>("count"),
                std::make_shared<ConstantExpression>(static_cast<uint64_t>(100000))));

        auto proj_2 = std::make_unique<Projector>(
            std::move(count_filter),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("CounterID"), "CounterID"},
                {std::make_shared<BinaryExpression>(
                     BinaryExpressionType::Div, std::make_shared<SelectExpression>("sum_strlen"),
                     std::make_shared<SelectExpression>("count")),
                 "avg"},
                {std::make_shared<SelectExpression>("count"), "count"}});

        return std::make_unique<TopK>(
            std::move(proj_2),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("avg"), "avg"}}, 25, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"Referer", Type::String}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("Referer"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        auto proj_1 = std::make_unique<Projector>(
            std::move(filter),
            std::vector<ProjectionMeta>{
                {std::make_shared<RegexExpression>(std::make_shared<SelectExpression>("Referer"),
                                                   R"(^https?://(?:www\.)?([^/]+)/.*$)", "$1"),
                 "k"},
                {std::make_shared<StrLenExpression>(std::make_shared<SelectExpression>("Referer")),
                 "strlen"},
                {std::make_shared<SelectExpression>("Referer"), "Referer"}});

        auto agg = std::make_unique<Aggregation>(
            std::move(proj_1),
            std::vector<AggregationMeta>{
                {AggregationType::Sum, std::make_shared<SelectExpression>("strlen"), "sum_strlen"},
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
                {AggregationType::Min, std::make_shared<SelectExpression>("Referer"),
                 "min_referer"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("k"), "k"}});

        auto having = std::make_unique<Filter>(
            std::move(agg),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Gt, std::make_shared<SelectExpression>("c"),
                std::make_shared<ConstantExpression>(static_cast<uint64_t>(100000))));

        auto proj_2 = std::make_unique<Projector>(
            std::move(having),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("k"), "k"},
                {std::make_shared<BinaryExpression>(
                     BinaryExpressionType::Div, std::make_shared<SelectExpression>("sum_strlen"),
                     std::make_shared<SelectExpression>("c")),
                 "l"},
                {std::make_shared<SelectExpression>("c"), "c"},
                {std::make_shared<SelectExpression>("min_referer"), "min_referer"},
            });

        return std::make_unique<TopK>(
            std::move(proj_2), std::vector<SortKey>{{std::make_shared<SelectExpression>("l"), "l"}},
            25, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"ResolutionWidth", Type::Int16}}));

        auto agg = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Sum, std::make_shared<SelectExpression>("ResolutionWidth"),
                 "sum"},
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"}},
            std::vector<GroupByMeta>{});

        std::vector<ProjectionMeta> meta(90);

        for (int i = 0; i < 90; i++) {
            std::string name = "sum + " + std::to_string(i);

            if (i == 0) {
                meta[i] = ProjectionMeta{std::make_shared<SelectExpression>("sum"), name};
            } else {
                meta[i] = ProjectionMeta{
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Add, std::make_shared<SelectExpression>("sum"),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Mul, std::make_shared<SelectExpression>("c"),
                            std::make_shared<ConstantExpression>(i))),
                    name};
            }
        }

        return std::make_unique<Projector>(std::move(agg), std::move(meta));
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"SearchEngineID", Type::Int16},
                                                             {"ClientIP", Type::Int32},
                                                             {"IsRefresh", Type::Int16},
                                                             {"ResolutionWidth", Type::Int16},
                                                             {"SearchPhrase", Type::String}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("SearchPhrase"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("IsRefresh"),
                 "sum_refresh"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("ResolutionWidth"),
                 "sum_width"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("SearchEngineID"), "SearchEngineID"},
                {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"}});

        auto proj = std::make_unique<Projector>(
            std::move(agg),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("SearchEngineID"), "SearchEngineID"},
                {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"},
                {std::make_shared<SelectExpression>("c"), "c"},
                {std::make_shared<SelectExpression>("sum_refresh"), "sum_refresh"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Div,
                                                    std::make_shared<SelectExpression>("sum_width"),
                                                    std::make_shared<SelectExpression>("c")),
                 "avg_width"}});

        return std::make_unique<TopK>(
            std::move(proj), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"WatchID", Type::Int64},
                                                             {"ClientIP", Type::Int32},
                                                             {"IsRefresh", Type::Int16},
                                                             {"ResolutionWidth", Type::Int16},
                                                             {"SearchPhrase", Type::String}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::Neq, std::make_shared<SelectExpression>("SearchPhrase"),
                std::make_shared<ConstantExpression>(std::string_view(""))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("IsRefresh"),
                 "sum_refresh"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("ResolutionWidth"),
                 "sum_width"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("WatchID"), "WatchID"},
                                     {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"}});

        auto proj = std::make_unique<Projector>(
            std::move(agg),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("WatchID"), "WatchID"},
                {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"},
                {std::make_shared<SelectExpression>("c"), "c"},
                {std::make_shared<SelectExpression>("sum_refresh"), "sum_refresh"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Div,
                                                    std::make_shared<SelectExpression>("sum_width"),
                                                    std::make_shared<SelectExpression>("c")),
                 "avg_width"}});

        return std::make_unique<TopK>(
            std::move(proj), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({
                                                         {"WatchID", Type::Int64},
                                                         {"ClientIP", Type::Int32},
                                                         {"IsRefresh", Type::Int16},
                                                         {"ResolutionWidth", Type::Int16},
                                                     }));

        auto agg = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("IsRefresh"),
                 "sum_refresh"},
                {AggregationType::Sum, std::make_shared<SelectExpression>("ResolutionWidth"),
                 "sum_width"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("WatchID"), "WatchID"},
                                     {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"}});

        auto proj = std::make_unique<Projector>(
            std::move(agg),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("WatchID"), "WatchID"},
                {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"},
                {std::make_shared<SelectExpression>("c"), "c"},
                {std::make_shared<SelectExpression>("sum_refresh"), "sum_refresh"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Div,
                                                    std::make_shared<SelectExpression>("sum_width"),
                                                    std::make_shared<SelectExpression>("c")),
                 "avg_width"}});

        return std::make_unique<TopK>(
            std::move(proj), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({
                                                         {"URL", Type::String},
                                                     }));

        auto agg = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
            },
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("URL"), "URL"}});

        return std::make_unique<TopK>(
            std::move(agg), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({
                                                         {"URL", Type::String},
                                                     }));
        auto agg = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
            },
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("URL"), "URL"}});

        auto proj = std::make_unique<Projector>(
            std::move(agg), std::vector<ProjectionMeta>{
                                {std::make_shared<ConstantExpression>(1), "one"},
                                {std::make_shared<SelectExpression>("URL"), "URL"},
                                {std::make_shared<SelectExpression>("c"), "c"},
                            });

        return std::make_unique<TopK>(
            std::move(proj), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({
                                                         {"ClientIP", Type::Int32},
                                                     }));

        auto agg = std::make_unique<Aggregation>(
            std::move(scan),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "c"},
            },
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("ClientIP"), "ClientIP"}});

        auto sort = std::make_unique<TopK>(
            std::move(agg), std::vector<SortKey>{{std::make_shared<SelectExpression>("c"), "c"}},
            10, false);

        return std::make_unique<Projector>(
            std::move(sort),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("ClientIP"), "ClientIP"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Add,
                                                    std::make_shared<SelectExpression>("ClientIP"),
                                                    std::make_shared<ConstantExpression>(-1)),
                 "ClientIP_1"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Add,
                                                    std::make_shared<SelectExpression>("ClientIP"),
                                                    std::make_shared<ConstantExpression>(-2)),
                 "ClientIP_2"},
                {std::make_shared<BinaryExpression>(BinaryExpressionType::Add,
                                                    std::make_shared<SelectExpression>("ClientIP"),
                                                    std::make_shared<ConstantExpression>(-3)),
                 "ClientIP_3"},
                {std::make_shared<SelectExpression>("c"), "c"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"URL", Type::String},
                                                             {"CounterID", Type::Int32},
                                                             {"Eventdate", Type::Date},
                                                             {"DontCountHits", Type::Int16},
                                                             {"IsRefresh", Type::Int16}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<SelectExpression>("Eventdate"),
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-01"))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-31")),
                            std::make_shared<SelectExpression>("Eventdate"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq,
                        std::make_shared<SelectExpression>("DontCountHits"),
                        std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsRefresh"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("CounterID"),
                            std::make_shared<ConstantExpression>(62))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Neq, std::make_shared<SelectExpression>("URL"),
                        std::make_shared<ConstantExpression>(std::string_view(""))))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("URL"), "URL"}});

        auto sort = std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("PageViews"), "PageViews"}},
            10, false);

        return std::make_unique<Projector>(
            std::move(sort), std::vector<ProjectionMeta>{
                                 {std::make_shared<SelectExpression>("URL"), "URL"},
                                 {std::make_shared<SelectExpression>("PageViews"), "PageViews"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"Title", Type::String},
                                                             {"CounterID", Type::Int32},
                                                             {"Eventdate", Type::Date},
                                                             {"DontCountHits", Type::Int16},
                                                             {"IsRefresh", Type::Int16}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<SelectExpression>("Eventdate"),
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-01"))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-31")),
                            std::make_shared<SelectExpression>("Eventdate"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq,
                        std::make_shared<SelectExpression>("DontCountHits"),
                        std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsRefresh"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("CounterID"),
                            std::make_shared<ConstantExpression>(62))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Neq, std::make_shared<SelectExpression>("Title"),
                        std::make_shared<ConstantExpression>(std::string_view(""))))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("Title"), "Title"}});

        auto sort = std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("PageViews"), "PageViews"}},
            10, false);

        return std::make_unique<Projector>(
            std::move(sort), std::vector<ProjectionMeta>{
                                 {std::make_shared<SelectExpression>("Title"), "Title"},
                                 {std::make_shared<SelectExpression>("PageViews"), "PageViews"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"URL", Type::String},
                                                             {"CounterID", Type::Int32},
                                                             {"Eventdate", Type::Date},
                                                             {"IsDownload", Type::Int16},
                                                             {"IsLink", Type::Int16},
                                                             {"IsRefresh", Type::Int16}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<SelectExpression>("Eventdate"),
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-01"))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-31")),
                            std::make_shared<SelectExpression>("Eventdate"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Neq, std::make_shared<SelectExpression>("IsLink"),
                        std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsRefresh"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsDownload"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq, std::make_shared<SelectExpression>("CounterID"),
                        std::make_shared<ConstantExpression>(62)))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("URL"), "URL"}});

        auto sort = std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("PageViews"), "PageViews"}},
            10, false, 1000);

        return std::make_unique<Projector>(
            std::move(sort), std::vector<ProjectionMeta>{
                                 {std::make_shared<SelectExpression>("URL"), "URL"},
                                 {std::make_shared<SelectExpression>("PageViews"), "PageViews"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"TraficSourceID", Type::Int16},
                                                             {"SearchEngineID", Type::Int16},
                                                             {"AdvEngineID", Type::Int16},
                                                             {"Referer", Type::String},
                                                             {"CounterID", Type::Int32},
                                                             {"Eventdate", Type::Date},
                                                             {"IsRefresh", Type::Int16},
                                                             {"URL", Type::String}}));
        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Geq, std::make_shared<SelectExpression>("Eventdate"),
                        std::make_shared<ConstantExpression>(cngn::Date("2013-07-01"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Geq,
                        std::make_shared<ConstantExpression>(cngn::Date("2013-07-31")),
                        std::make_shared<SelectExpression>("Eventdate"))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq, std::make_shared<SelectExpression>("CounterID"),
                        std::make_shared<ConstantExpression>(62)),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq, std::make_shared<SelectExpression>("IsRefresh"),
                        std::make_shared<ConstantExpression>(static_cast<int16_t>(0))))));

        auto case_proj = std::make_unique<Projector>(
            std::move(filter),
            std::vector<ProjectionMeta>{
                {std::make_shared<SelectExpression>("TraficSourceID"), "TraficSourceID"},
                {std::make_shared<SelectExpression>("SearchEngineID"), "SearchEngineID"},
                {std::make_shared<SelectExpression>("AdvEngineID"), "AdvEngineID"},
                {std::make_shared<CaseExpression>(
                     std::make_shared<BinaryExpression>(
                         BinaryExpressionType::And,
                         std::make_shared<BinaryExpression>(
                             BinaryExpressionType::Eq,
                             std::make_shared<SelectExpression>("SearchEngineID"),
                             std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                         std::make_shared<BinaryExpression>(
                             BinaryExpressionType::Eq,
                             std::make_shared<SelectExpression>("AdvEngineID"),
                             std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))),
                     std::make_shared<SelectExpression>("Referer"),
                     std::make_shared<ConstantExpression>(std::string_view(""))),
                 "Src"},
                {std::make_shared<SelectExpression>("URL"), "Dst"}});

        auto agg = std::make_unique<Aggregation>(
            std::move(case_proj),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("TraficSourceID"), "TraficSourceID"},
                {std::make_shared<SelectExpression>("SearchEngineID"), "SearchEngineID"},
                {std::make_shared<SelectExpression>("AdvEngineID"), "AdvEngineID"},
                {std::make_shared<SelectExpression>("Src"), "Src"},
                {std::make_shared<SelectExpression>("Dst"), "Dst"}});

        return std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("PageViews"), "PageViews"}},
            10, false, 1000);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"URLHash", Type::Int64},
                                                             {"Eventdate", Type::Date},
                                                             {"CounterID", Type::Int32},
                                                             {"TraficSourceID", Type::Int16},
                                                             {"RefererHash", Type::Int64},
                                                             {"IsRefresh", Type::Int16}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<SelectExpression>("Eventdate"),
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-01"))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Geq,
                            std::make_shared<ConstantExpression>(cngn::Date("2013-07-31")),
                            std::make_shared<SelectExpression>("Eventdate"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Or,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("TraficSourceID"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(-1))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("TraficSourceID"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(6))))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsRefresh"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("RefererHash"),
                            std::make_shared<ConstantExpression>(
                                static_cast<int64_t>(3594120000172545465ll)))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq, std::make_shared<SelectExpression>("CounterID"),
                        std::make_shared<ConstantExpression>(62)))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("URLHash"), "URLHash"},
                {std::make_shared<SelectExpression>("Eventdate"), "Eventdate"}});

        auto sort = std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("PageViews"), "PageViews"}},
            10, false, 100);

        return std::make_unique<Projector>(
            std::move(sort), std::vector<ProjectionMeta>{
                                 {std::make_shared<SelectExpression>("URLHash"), "URLHash"},
                                 {std::make_shared<SelectExpression>("Eventdate"), "Eventdate"},
                                 {std::make_shared<SelectExpression>("PageViews"), "PageViews"}});
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"WindowClientWidth", Type::Int16},
                                                             {"WindowClientHeight", Type::Int16},
                                                             {"CounterID", Type::Int32},
                                                             {"Eventdate", Type::Date},
                                                             {"IsRefresh", Type::Int16},
                                                             {"DontCountHits", Type::Int16},
                                                             {"URLHash", Type::Int64}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Geq, std::make_shared<SelectExpression>("Eventdate"),
                        std::make_shared<ConstantExpression>(cngn::Date("2013-07-01"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Geq,
                        std::make_shared<ConstantExpression>(cngn::Date("2013-07-31")),
                        std::make_shared<SelectExpression>("Eventdate"))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("CounterID"),
                            std::make_shared<ConstantExpression>(62)),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsRefresh"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("DontCountHits"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq, std::make_shared<SelectExpression>("URLHash"),
                            std::make_shared<ConstantExpression>(
                                static_cast<int64_t>(2868770270353813622ll)))))));

        auto agg = std::make_unique<Aggregation>(
            std::move(filter),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{
                {std::make_shared<SelectExpression>("WindowClientWidth"), "WindowClientWidth"},
                {std::make_shared<SelectExpression>("WindowClientHeight"), "WindowClientHeight"}});

        return std::make_unique<TopK>(
            std::move(agg),
            std::vector<SortKey>{{std::make_shared<SelectExpression>("PageViews"), "PageViews"}},
            10, false, 10000);
    },
    [](const std::string& filename) {
        auto scan = std::make_unique<Scan>(filename, Schema({{"EventTime", Type::Timestamp},
                                                             {"CounterID", Type::Int32},
                                                             {"Eventdate", Type::Date},
                                                             {"IsRefresh", Type::Int16},
                                                             {"DontCountHits", Type::Int16}}));

        auto filter = std::make_unique<Filter>(
            std::move(scan),
            std::make_shared<BinaryExpression>(
                BinaryExpressionType::And,
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Geq, std::make_shared<SelectExpression>("Eventdate"),
                        std::make_shared<ConstantExpression>(cngn::Date("2013-07-14"))),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Geq,
                        std::make_shared<ConstantExpression>(cngn::Date("2013-07-15")),
                        std::make_shared<SelectExpression>("Eventdate"))),
                std::make_shared<BinaryExpression>(
                    BinaryExpressionType::And,
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::Eq, std::make_shared<SelectExpression>("CounterID"),
                        std::make_shared<ConstantExpression>(62)),
                    std::make_shared<BinaryExpression>(
                        BinaryExpressionType::And,
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("IsRefresh"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0))),
                        std::make_shared<BinaryExpression>(
                            BinaryExpressionType::Eq,
                            std::make_shared<SelectExpression>("DontCountHits"),
                            std::make_shared<ConstantExpression>(static_cast<int16_t>(0)))))));

        auto proj = std::make_unique<Projector>(
            std::move(filter),
            std::vector<ProjectionMeta>{
                {std::make_shared<ExtractMinute>(std::make_shared<SelectExpression>("EventTime")),
                 "M"}});

        auto agg = std::make_unique<Aggregation>(
            std::move(proj),
            std::vector<AggregationMeta>{
                {AggregationType::Count, std::make_shared<ConstantExpression>(0), "PageViews"}},
            std::vector<GroupByMeta>{{std::make_shared<SelectExpression>("M"), "M"}});

        return std::make_unique<TopK>(
            std::move(agg), std::vector<SortKey>{{std::make_shared<SelectExpression>("M"), "M"}},
            10, true, 1000);
    }};
