#pragma once

#include <string>
#include <vector>

#include "BatchedWriter.h"

struct DefaultTestConfig {
    static cngn::Batch DefaultPrepare() {
        cngn::BatchedWriter writer(kFilename, kDefaultSchema);
        writer.WriteBatch(k_default_batch);
        writer.WriteMetadata();
        writer.Flush();

        return cngn::Batch(
            std::vector{cngn::Column(std::vector<int64_t>{1, 5, 8}),
                        cngn::Column(std::vector<int64_t>{-2, 1, 17}),
                        cngn::Column(std::vector<std::string_view>{"first", "second", "third"}),
                        cngn::Column(std::vector<int64_t>{4, 2, 2})},
            kDefaultSchema);
    }

    static constexpr std::string kFilename = "test.chsv";

    inline static const auto kDefaultSchema = cngn::Schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::Int64},
    }});

    inline static auto k_default_batch = cngn::Batch(
        std::vector{cngn::Column(std::vector<int64_t>{1, 5, 8}),
                    cngn::Column(std::vector<int64_t>{-2, 1, 17}),
                    cngn::Column(std::vector<std::string_view>{"first", "second", "third"}),
                    cngn::Column(std::vector<int64_t>{4, 2, 2})},
        kDefaultSchema);
};
