#pragma once

#include <fstream>
#include <string>
#include <vector>

// #include "Batch.h"
#include "CsvReader.h"

using Row = std::vector<std::string>;

class StringCSVConverter {
public:
    static void StringsToCsv(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream ofs(filename);

        if (!ofs.is_open()) {
            throw std::runtime_error("Could not open file for writing");
        }

        for (const auto& line : lines) {
            ofs << line << std::endl;
        }
    }
};

inline std::vector<Row> ReadAllLines(cngn::CsvReader& reader) {
    std::vector<Row> rows;

    size_t rows_cnt = 0;

    while (reader.ReadLine()) {
        ++rows_cnt;
    }

    static auto chunk = reader.GetChunk();
    chunk.InitColumnsCnt(rows_cnt);

    rows.resize(rows_cnt);

    for (size_t i = 0; i < rows_cnt; ++i) {
        for (size_t j = 0; j < chunk.GetColsCount(rows_cnt); ++j) {
            auto field = chunk.GetField(i, j);
            rows[i].emplace_back(std::string(field));
        }
    }

    return rows;
}