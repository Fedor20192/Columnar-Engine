#pragma once

#include <string>
#include <vector>

#include "CsvReader.h"
#include "Types.h"
#include "glog/logging.h"

namespace cngn {

class Schema {
public:
    struct ColumnData {
        std::string column_name;
        Type column_type;
    };

    explicit Schema(const std::vector<ColumnData> &other_data) : schema_(other_data) {
    }

    static Schema ReadFromFile(const std::string &file_name) {
        CsvReader reader(file_name);

        auto rows = reader.ReadAllLines();
        std::vector<ColumnData> data;
        data.reserve(rows.size());
        for (auto &row : rows) {
            if (row.size() != 2) {
                DLOG(FATAL) << "Wrong number of rows in file: " << file_name << '\n'
                            << "Rows count: " << rows.size() << std::endl;
                throw std::runtime_error("Wrong number of rows in file: " + file_name);
            }

            data.emplace_back(row[0], DeserializeType(row[1]));
        }

        return Schema(data);
    }

    void WriteToFile(const std::string &file_name) const {
        std::ofstream file(file_name);

        for (size_t i = 0; i < schema_.size(); i++) {
            file << schema_[i].column_name << ',' << SerializeType(schema_[i].column_type) << '\n';
        }
    }

    const std::vector<ColumnData> &GetData() const {
        return schema_;
    }

private:
    std::vector<ColumnData> schema_;
};

}  // namespace cngn
