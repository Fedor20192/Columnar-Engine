#include "Schema.h"

#include <fstream>

#include "CsvReader.h"
#include "glog/logging.h"

namespace cngn {
Schema::Schema(const std::vector<ColumnData>& other_data) : schema_(other_data) {
}

Schema::Schema(std::vector<ColumnData>&& other_data) noexcept : schema_(std::move(other_data)) {
}

Schema Schema::ReadFromCsv(const std::string& file_name) {
    CsvReader reader(file_name);

    auto rows = reader.ReadAllLines();
    std::vector<ColumnData> data;
    data.reserve(rows.size());
    for (size_t i = 0; i < rows.size(); i++) {
        const auto& row = rows[i];
        if (row.size() != 2) {
            throw std::runtime_error("[Schema]: Wrong number of columns in file: " + file_name + '\n' +
                                     "Columns count: " + std::to_string(row.size()) + '\n' +
                                     "Line number " + std::to_string(i));
        }

        data.emplace_back(std::move(row[0]), DeserializeType(row[1]));
    }

    return Schema(data);
}

void Schema::WriteToFile(const std::string& file_name) const {
    std::ofstream file(file_name);

    for (size_t i = 0; i < schema_.size(); i++) {
        file << schema_[i].column_name << ','
             << DispatchOnType(schema_[i].column_type, SerializeType()) << '\n';
    }
}

const std::vector<Schema::ColumnData>& Schema::GetData() const {
    return schema_;
}

std::vector<PhysTypeVariant> Schema::Serialize() const {
    std::vector<PhysTypeVariant> result;
    result.reserve(schema_.size() * 2 + 1);

    result.push_back(schema_.size());

    for (size_t i = 0; i < schema_.size(); i++) {
        result.emplace_back(schema_[i].column_name);
        result.emplace_back(DispatchOnType(schema_[i].column_type, SerializeType()));
    }

    return result;
}

size_t Schema::GetColumnsCount() const {
    return schema_.size();
}

const Schema::ColumnData& Schema::operator[](size_t index) const {
    return schema_[index];
}

}  // namespace cngn