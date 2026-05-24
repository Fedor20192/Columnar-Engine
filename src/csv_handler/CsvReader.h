#pragma once

#include <memory>
#include <string>
#include <vector>

#include "InputBuffer.h"

namespace cngn {
class CsvReader {
public:
    class Chunk {
    public:
        Chunk();

        Chunk(const Chunk& chunk) = delete;
        Chunk(Chunk&& chunk) = default;
        Chunk& operator=(const Chunk& chunk) = delete;
        Chunk& operator=(Chunk&& chunk) = default;

        void Add(std::string_view);
        void AddSimple(std::string_view);
        void Prepare();
        void Reset();
        void InitColumnsCnt(size_t rows_cnt);
        std::string_view GetField(size_t row_ind, size_t col_ind) const;
        size_t GetColsCount(size_t rows_cnt) const;
        bool Empty() const;
        std::pair<std::shared_ptr<std::vector<char>>, std::shared_ptr<MmapRegion>> GetBuffer();
        void SetRegion(const std::shared_ptr<MmapRegion>& region);

    private:
        struct FieldMeta {
            size_t idx, offset, size;
        };

        std::vector<FieldMeta> fields_meta_;
        std::vector<std::string_view> fields_;
        std::shared_ptr<std::vector<char>> buffer_;
        std::shared_ptr<MmapRegion> region_;
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
        LineState() = default;

        void Reset();

        bool need_break = false;
        bool has_read = false;
        bool is_valid = true;
        struct FieldState {
            FieldState() = default;

            std::string data{};
            std::string_view direct{};
            bool is_quote_open = false;
            bool is_quote_close = false;

            void Reset();
            bool IsSimple() const;
        };
        FieldState field{};
    };

    LineState state_;

    void FieldHandler(int c, LineState& line_state);

    InputBuffer buffer_;
    Chunk chunk_;
};
}  // namespace cngn
