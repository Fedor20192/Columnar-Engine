#pragma once

#include <fstream>
#include <optional>
#include <vector>
#include <string>

namespace cngn {
class CsvReader {
public:
    struct Parameters {
        Parameters() {
        }

        char delimiter = ',';
        char quote = '"';
        char linebreak = '\n';
    };

    explicit CsvReader(const std::string& filename, Parameters params = Parameters{});

    CsvReader(const CsvReader&) = delete;
    CsvReader& operator=(const CsvReader&) = delete;
    CsvReader(CsvReader&&) = default;
    CsvReader& operator=(CsvReader&&) = default;

    using Row = std::vector<std::string>;

    std::optional<Row> ReadLine();

private:
    Parameters parameters_;
    std::ifstream file_;

    struct LineState {
        LineState() {
        }

        Row row;
        bool need_break = false;
        bool has_read = false;
        bool is_valid = true;
        struct FieldState {
            FieldState() {
            }

            bool is_quote_open = false;
            bool is_quote_close = false;
            std::string data{};
        };
        FieldState field{};
    };

    void FieldHandler(char c, LineState& line_state);
};
}  // namespace cngn
