#pragma once

#include <functional>

#include "../operators/Count.h"
#include "../operators/Expression.h"
#include "../operators/Filter.h"
#include "../operators/Scan.h"

using QueryGenerator = std::function<std::unique_ptr<cngn::Operator>(const std::string&)>;

constexpr int kQueriesCount = 2;

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
    }

};
