#include "Scan.h"

#include "glog/logging.h"

namespace cngn {

Scan::Scan(const std::string& filename, Schema need_columns_schema)
    : reader_(filename), schema_(std::move(need_columns_schema)) {
}

void Scan::Open() {
    DLOG(INFO) << "[Scan]: Starting opening\n";

    const auto& meta = reader_.GetMetadata().GetSchema().GetData();

    const auto& need_columns_names = schema_.GetData();

    std::vector<std::pair<size_t, std::string>> selected_columns;
    selected_columns.reserve(need_columns_names.size());

    if (need_columns_names.empty()) {
        DLOG(WARNING) << "[Scan]: Empty context";

        if (!meta.empty()) {
            DLOG(INFO) << "[Scan]: Selecting default column with name " << meta[0].column_name
                       << '\n';
            selected_columns.emplace_back(
                0,
                meta[0].column_name);  // todo: Допилить выбор дефолтной колонки наименьшего размера
        }
    }

    for (const auto& [column_name, type] : need_columns_names) {
        bool found = false;
        for (size_t i = 0; i < meta.size() && !found; i++) {
            const auto& column = meta[i];
            if (column.column_name == column_name) {
                selected_columns.emplace_back(i, column_name);
                found = true;
            }
        }

        if (!found) {
            throw std::runtime_error("[Scan]: Unknown column " + column_name);
        }
    }

    std::sort(selected_columns.begin(), selected_columns.end());

    std::vector<uint64_t> column_indices;
    column_indices.reserve(need_columns_names.size());

    Context::Mapping columns_mapping;
    columns_mapping.reserve(need_columns_names.size());

    for (size_t i = 0; i < selected_columns.size(); i++) {
        columns_mapping[selected_columns[i].second] = column_indices.size();
        column_indices.push_back(selected_columns[i].first);
    }

    reader_.SetIndices(std::move(column_indices));

    DLOG(INFO) << "[Scan]: Successfully opened\n";
}

std::optional<Batch> Scan::Next() {
    return reader_.ReadBatch();
}

void Scan::Close() {
}

}  // namespace cngn