#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace cngn {

class Schema {
public:
    struct ColumnData {
        std::string column_name;
        Type column_type;

        bool operator==(const ColumnData &other) const = default;
    };

    Schema() = default;
    explicit Schema(const std::vector<ColumnData> &other_data);
    explicit Schema(std::vector<ColumnData> &&other_data) noexcept;

    static Schema ReadFromCsv(const std::string &file_name);

    void WriteToFile(const std::string &file_name) const;

    const std::vector<ColumnData> &GetData() const;

    std::vector<PhysTypeVariant> Serialize() const;

    size_t GetColumnsCount() const;

    const ColumnData& operator[](size_t index) const;

    bool operator==(const Schema &other) const = default;

private:
    std::vector<ColumnData> schema_;
};

}  // namespace cngn
