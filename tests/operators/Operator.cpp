#include "Operator.h"

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "BatchedWriter.h"
#include "Count.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Require context", "[VirtualOperator]") {

    std::ofstream out(DefaultTestConfig::kFilename);
    DefaultTestConfig::DefaultPrepare();

    auto scan = std::make_unique<cngn::Scan>(DefaultTestConfig::kFilename, nullptr);
    REQUIRE_THROWS(scan->Open());

    auto count_1 = std::make_unique<cngn::Count>(std::move(scan), nullptr);
    REQUIRE_THROWS(count_1->Open());
}