#pragma once

#include <string>
#include <vector>

#include "BatchedWriter.h"

inline cngn::Batch DefaultPrepare(const std::string &filename) {
    cngn::Schema schema({std::vector<cngn::Schema::ColumnData>{
        {"a", cngn::Type::Int64},
        {"b", cngn::Type::Int64},
        {"name123", cngn::Type::String},
        {"d", cngn::Type::Int64},
    }});

    cngn::Batch batch(schema);
    batch.AddColumn(cngn::Column(std::vector<int64_t>{1, 5, 8}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{2, 1, 17}));
    batch.AddColumn(cngn::Column(std::vector<std::string_view>{"first", "second", "third"}));
    batch.AddColumn(cngn::Column(std::vector<int64_t>{4, 2, 2}));

    cngn::BatchedWriter writer(filename, schema);
    writer.WriteBatch(batch);
    writer.WriteMetadata();
    writer.Flush();

    return batch;
}
