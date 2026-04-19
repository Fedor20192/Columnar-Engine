#include "Scan.h"

#include <unordered_set>

#include "glog/logging.h"

namespace cngn {

Scan::Scan(const std::string& filename, const std::shared_ptr<Context>& context)
    : Operator(context), reader_(filename) {
}

void Scan::Open() {
    DLOG(INFO) << "Scan::Open\n";

    if (!context_) {
        DLOG(ERROR) << "No context";
        throw std::runtime_error("No context");
    }

    if (context_->GetNames().empty()) {
        DLOG(WARNING) << "Empty context";
    }

    const auto& meta = reader_.GetMetadata().GetSchema().GetData();

    const auto& need_columns_names = context_->GetNames();

    struct ColumnInfo {
        uint32_t index;
        std::string name;
    };

    std::vector<ColumnInfo> selected_columns;
    selected_columns.reserve(need_columns_names.size());

    for (const auto& column_name : need_columns_names) {
        bool found = false;
        for (size_t i = 0; i < meta.size() && !found; i++) {
            const auto& column = meta[i];
            if (column.column_name == column_name) {
                selected_columns.emplace_back(i, column_name);
                found = true;
            }
        }

        if (!found) {
            DLOG(ERROR) << "Unknown column  '" << column_name << "'\n";
            throw std::runtime_error("Unknown column " + column_name);
        }
    }

    std::sort(selected_columns.begin(), selected_columns.end(),
              [](const ColumnInfo& a, const ColumnInfo& b) { return a.index < b.index; });

    std::vector<uint64_t> column_indices;
    column_indices.reserve(need_columns_names.size());

    Context::Mapping columns_mapping;
    columns_mapping.reserve(need_columns_names.size());

    for (size_t i = 0; i < selected_columns.size(); i++) {
        columns_mapping[selected_columns[i].name] = column_indices.size();
        column_indices.push_back(selected_columns[i].index);
    }

    reader_.SetIndices(std::move(column_indices));
    context_->SetMapping(std::move(columns_mapping));

    DLOG(INFO) << "Finalize Scan::Open\n";
}

std::optional<Batch> Scan::Next() {
    return reader_.ReadBatch();
}

void Scan::Close() {
}

}  // namespace cngn