#pragma once

#include <fstream>
#include <memory>
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

    class Chunk {
    public:
        Chunk();

        Chunk(const Chunk& chunk) = delete;
        Chunk(Chunk&& chunk) = default;
        Chunk& operator=(const Chunk& chunk) = delete;
        Chunk& operator=(Chunk&& chunk) = default;

        void Add(const std::string&);
        void Prepare();
        void Reset();
        void InitColumnsCnt(size_t rows_cnt);
        std::string_view GetField(size_t row_ind, size_t col_ind) const;
        size_t GetColsCount(size_t rows_cnt) const;
        bool Empty() const;
        std::shared_ptr<std::vector<char>> GetBuffer();

    private:
        std::vector<size_t> fields_sizes_;
        std::vector<std::string_view> fields_;
        std::shared_ptr<std::vector<char>> buffer_;
        size_t cols_cnt_;
        bool is_prepared_{false};
    };

    explicit CsvReader(const std::string& filename);

    CsvReader(const CsvReader&) = delete;
    CsvReader& operator=(const CsvReader&) = delete;
    CsvReader(CsvReader&&) = default;
    CsvReader& operator=(CsvReader&&) = default;

    Chunk GetChunk();
    bool ReadLine();

private:
    struct LineState {
        LineState() {
        }

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

    class InputBuffer {
    public:
        explicit InputBuffer(const std::string& filename);
        int GetChar();
        int Peek();
        std::string_view FindSymb(char symb) const;
        std::string_view FindDels() const;
        void UpdatePos(size_t plus);
        size_t GetSize() const;
        size_t GetPos() const;

    private:
        static constexpr size_t kBufCp = 11 * 1024 * 1024 + 64;
        std::ifstream file_;

        std::unique_ptr<char[]> buffer_;
        size_t buffer_pos_ = 0, buffer_sz_ = 0;

        void Update();
    };

    void FieldHandler(int c, LineState& line_state);

    InputBuffer buffer_;
    Chunk chunk_;
};
}  // namespace cngn
