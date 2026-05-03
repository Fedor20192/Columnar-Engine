#pragma once

#include <functional>

#include "../operators/Count.h"
#include "../operators/Scan.h"

using QueryGenerator = std::function<std::unique_ptr<cngn::Operator>(const std::string&)>;

constexpr int kQueriesCount = 1;

const std::array<QueryGenerator, kQueriesCount> kGenerators = {
    [](const std::string& filename) {
        return std::make_unique<cngn::Count>(std::make_unique<cngn::operators::Scan>(filename, cngn::Schema()));
    },
};