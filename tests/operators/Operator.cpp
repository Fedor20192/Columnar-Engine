#include "Operator.h"

#include "../Fixtures.h"
#include "../utils/Prepare.h"
#include "BatchedWriter.h"
#include "Count.h"
#include "Scan.h"
#include "catch2/catch_template_test_macros.hpp"

TEST_CASE_METHOD(GlogFixture, "Require context", "[VirtualOperator]") {
    const std::string filename = "test.chsv";

    std::ofstream out(filename);
    DefaultPrepare(filename);

    auto scan = std::make_unique<cngn::Scan>(filename, nullptr);
    REQUIRE_THROWS(scan->Open());

    auto count_1 = std::make_unique<cngn::Count>(std::move(scan), nullptr);
    REQUIRE_THROWS(count_1->Open());
}