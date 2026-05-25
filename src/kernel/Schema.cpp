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

    size_t rows_cnt = 0;
    while (reader.ReadLine()) {
        ++rows_cnt;
    }

    auto chunk = reader.GetChunk();
    chunk.InitColumnsCnt(rows_cnt);

    std::vector<ColumnData> data;
    for (size_t i = 0; i < rows_cnt; i++) {
        data.emplace_back(
            std::string(chunk.GetField(i, 0)),
            DeserializeType(std::string(chunk.GetField(i, 1)))
        );
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
    const size_t sz = schema_.size();
    result.reserve(sz * 2 + 1);

    result.emplace_back(sz);

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