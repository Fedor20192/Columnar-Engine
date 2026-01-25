#include <string>
#include <vector>

#include "../Fixtures.h"
#include "BatchedReader.h"
#include "BatchedWriter.h"
#include "CsvWriter.h"
#include "catch2/catch_template_test_macros.hpp"

using Row = cngn::CsvWriter::Row;

TEST_CASE_METHOD(GlogFixture, "CrossValidation", "[BatchedRW]") {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::Int64},
    }});

    cngn::Batch batch(schema);
    batch.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{2, 1, 17}));
    batch.AddColumn(cngn::Column(std::vector<std::string>{"first", "second", "third"}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{4, 2, 2}));

    std::string filename("test.chsv");
    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();


    cngn::BatchedReader reader(filename);
    const cngn::Metadata& metadata = reader.GetMetadata();
    const int64_t batch_cnt = metadata.GetBatchCnt();

    REQUIRE(batch_cnt == 1);

    for (int64_t i = 0; i < batch_cnt; i++) {
        auto read_batch = reader.ReadBatch(static_cast<size_t>(i));

        REQUIRE(read_batch.has_value());

        auto real_batch = read_batch.value();

        REQUIRE(real_batch.ColumnCount() == 4);
        REQUIRE(real_batch[0] == cngn::Column(std::vector<int64_t>{1, 5, 8}));
        REQUIRE(real_batch[1] == cngn::Column(std::vector<int64_t>{2, 1, 17}));
        REQUIRE(real_batch[2] == cngn::Column(std::vector<std::string>{"first", "second", "third"}));
        REQUIRE(real_batch[3] == cngn::Column(std::vector<int64_t>{4, 2, 2}));
    }

    REQUIRE(!reader.ReadBatch(batch_cnt).has_value());
}