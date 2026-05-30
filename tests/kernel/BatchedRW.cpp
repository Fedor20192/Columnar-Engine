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
        REQUIRE(real_batch[1] == cngn::Column(std::vector<int64_t>{-2, 1, 17}));
        REQUIRE(real_batch[2] == cngn::Column(std::vector<std::string_view>{"first", "second", "third"}));
        REQUIRE(real_batch[3] == cngn::Column(std::vector<int64_t>{4, 2, 2}));
    }

    REQUIRE(!reader.ReadBatch().has_value());
}

TEST_CASE_METHOD(GlogFixture, "Two batches first column offset", "[BatchedRW]") {
    cngn::Schema schema(std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
    });

    cngn::Batch batch_1(std::vector{
        cngn::Column{std::vector<int64_t>{1, 2, 3}},
        cngn::Column{std::vector<int64_t>{10, 20, 30}},
    }, schema);

    cngn::Batch batch_2(std::vector{
        cngn::Column{std::vector<int64_t>{4, 5, 6}},
        cngn::Column{std::vector<int64_t>{40, 50, 60}},
    }, schema);

    cngn::BatchedWriter writer(DefaultTestConfig::kFilename, schema);
    writer.WriteBatch(batch_1);
    writer.WriteBatch(batch_2);
    writer.WriteMetadata();
    writer.Flush();

    cngn::BatchedReader reader(DefaultTestConfig::kFilename);
    REQUIRE(reader.GetMetadata().GetBatchCnt() == 2);

    auto b1 = reader.ReadBatch();
    REQUIRE(b1.has_value());
    REQUIRE(b1->ColumnCount() == 2);
    REQUIRE((*b1)[0] == cngn::Column(std::vector<int64_t>{1, 2, 3}));
    REQUIRE((*b1)[1] == cngn::Column(std::vector<int64_t>{10, 20, 30}));

    auto b2 = reader.ReadBatch();
    REQUIRE(b2.has_value());
    REQUIRE(b2->ColumnCount() == 2);
    REQUIRE((*b2)[0] == cngn::Column(std::vector<int64_t>{4, 5, 6}));
    REQUIRE((*b2)[1] == cngn::Column(std::vector<int64_t>{40, 50, 60}));

    REQUIRE(!reader.ReadBatch().has_value());
}