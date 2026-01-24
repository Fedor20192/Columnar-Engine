#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace cngn {
class CsvWriter {
public:
    struct Parameters {
        Parameters() {
        }

        char delimiter = ',';
        char quote = '"';
        char linebreak = '\n';
    };

    explicit CsvWriter(const std::string &filename, Parameters parameters = Parameters{});

    CsvWriter(const CsvWriter &) = delete;
    CsvWriter &operator=(const CsvWriter &) = delete;
    CsvWriter(CsvWriter &&) = default;
    CsvWriter &operator=(CsvWriter &&) = default;

    using Row = std::vector<std::string>;

    void WriteRow(const Row &row);
    void WriteAllRows(const std::vector<Row> &rows);

    void Flush();

private:
    std::ofstream file_;
    Parameters parameters_;

    std::string StringToField(const std::string &str) const;
};
}  // namespace cngn
