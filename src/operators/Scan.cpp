#include "Scan.h"
#include "glog/logging.h"

#include <unordered_map>

namespace cngn {

Scan::Scan(const std::string& filename) : reader_(filename) {
}

Scan::Scan(const std::string& filename, const std::vector<std::string>& columns_names)
    : reader_(filename) {
    const auto& schema_data = reader_.GetMetadata().GetSchema().GetData();
    std::unordered_map<std::string_view, uint64_t> schema_names;
    schema_names.reserve(schema_data.size());

    uint64_t schema_index = 0;
    for (const auto& [column_name, type]: schema_data) {
        schema_names[column_name] = schema_index++;
    }

    std::vector<uint64_t> columns_indices;
    columns_indices.reserve(columns_names.size());

    for (const auto& column_name : columns_names) {
        if (!schema_names.contains(column_name)) {
            DLOG(ERROR) << "Unknown column name '" << column_name << "\n";
            throw std::runtime_error("Unknown column name");
        }
        columns_indices.push_back(schema_names[column_name]);
    }
    std::sort(columns_indices.begin(), columns_indices.end());
    reader_.SetIndices(std::move(columns_indices));
}

void Scan::Open() {
}

std::optional<Batch> Scan::Next() {
    return reader_.ReadBatch();
}

void Scan::Close() {
}

}  // namespace cngn