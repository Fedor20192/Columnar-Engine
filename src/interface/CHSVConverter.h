#pragma once

#include <string>

namespace cngn {
void FromCsvToFormat(const std::string &schema_name, const std::string &source_name,
                     const std::string &table_name);
void FromFormatToCsv(const std::string &table_name, const std::string &target_name);
}  // namespace cngn