#include <string>
#include <vector>

#include "../Fixtures.h"
#include "BatchedReader.h"
#include "BatchedWriter.h"
#include "CsvWriter.h"
#include "catch2/catch_template_test_macros.hpp"
#include "../utils/Prepare.h"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "Batched RW CrossValidation", "[BatchedRW]") {
    DefaultTestConfig::DefaultPrepare();

    cngn::BatchedReader reader(DefaultTestConfig::kFilename);
    const cngn::Metadata& metadata = reader.GetMetadata();
    const int64_t batch_cnt = metadata.GetBatchCnt();

    REQUIRE(batch_cnt == 1);

    for (int64_t i = 0; i < batch_cnt; i++) {
        auto read_batch = reader.ReadBatch();

        REQUIRE(read_batch.has_value());

        auto real_batch = std::move(read_batch.value());

        REQUIRE(real_batch.ColumnCount() == 4);
        REQUIRE(real_batch[0] == cngn::Column(std::vector<int64_t>{1, 5, 8}));
        REQUIRE(real_batch[1] == cngn::Column(std::vector<int64_t>{2, 1, 17}));
        REQUIRE(real_batch[2] == cngn::Column(std::vector<std::string_view>{"first", "second", "third"}));
        REQUIRE(real_batch[3] == cngn::Column(std::vector<int64_t>{4, 2, 2}));
    }

    REQUIRE(!reader.ReadBatch().has_value());
}