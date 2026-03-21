#pragma once

#include <array>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace cngn {
class CsvReader {
public:
    struct Parameters {
        Parameters() {
        }

        static constexpr char kDelimiter = ',';
        static constexpr char kQuote = '"';
        static constexpr char kLinebreak = '\n';
    };

    explicit CsvReader(const std::string& filename);

    CsvReader(const CsvReader&) = delete;
    CsvReader& operator=(const CsvReader&) = delete;
    CsvReader(CsvReader&&) = default;
    CsvReader& operator=(CsvReader&&) = default;

    using Row = std::vector<std::string>;

    std::optional<Row> ReadLine();
    std::vector<Row> ReadAllLines();

private:

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

            void Reset();
        };
        FieldState field{};
    };

    class Buffer {
    public:
        explicit Buffer(const std::string& filename);
        int GetChar();
        int Peek();
        std::string_view FindSymb(char symb);
        std::string_view FindDels() const;
        void UpdatePos(size_t plus);
        size_t GetSize() const;
        size_t GetPos() const;
    private:
        static constexpr size_t kBufCp = 1024 * 1024 + 64;
        std::ifstream file_;

        std::array<char, kBufCp> buffer_;
        size_t buffer_pos_ = 0, buffer_sz_ = 0;

        void Update();
        friend Parameters;
    };

    Buffer buffer_;

    void FieldHandler(int c, LineState& line_state);
};
}  // namespace cngn
