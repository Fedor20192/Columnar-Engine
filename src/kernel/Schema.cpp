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
    for (auto& row : rows) {
        if (row.size() != 2) {
            DLOG(FATAL) << "Wrong number of rows in file: " << file_name << '\n'
                        << "Rows count: " << rows.size() << std::endl;
            throw std::runtime_error("Wrong number of rows in file: " + file_name);
        }

        data.emplace_back(row[0], DeserializeType(row[1]));
    }

    return Schema(data);
}

void Schema::WriteToFile(const std::string& file_name) const {
    std::ofstream file(file_name);

    for (size_t i = 0; i < schema_.size(); i++) {
        file << schema_[i].column_name << ',' << SerializeType(schema_[i].column_type) << '\n';
    }
}

const std::vector<Schema::ColumnData>& Schema::GetData() const {
    return schema_;
}

std::vector<PhysTypeVariant> Schema::Serialize() const {
    std::vector<PhysTypeVariant> result;
    result.reserve(schema_.size() * 2 + 1);

    result.push_back(static_cast<int64_t>(schema_.size()));

    for (size_t i = 0; i < schema_.size(); i++) {
        result.push_back(schema_[i].column_name);
        result.push_back(SerializeType(schema_[i].column_type));
    }

    return result;
}

}  // namespace cngn