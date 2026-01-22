#pragma once

#include "BatchedCsvReader.h"
#include "CsvReader.h"
#include "CsvWriter.h"

#include <fstream>
#include <optional>
#include <string>
#include <vector>

class Converter {
public:
    using Row = cngn::CsvWriter::Row;
    using Batch = cngn::BatchedCsvReader::Batch;

    static void RowsToCsv(const std::string& filename, const std::vector<Row>& rows) {
        cngn::CsvWriter writer(filename);

        for (const auto& row : rows) {
            writer.WriteRow(row);
        }
    }

    static std::vector<Row> StringsFromReader(const std::string& filename) {
        std::vector<Row> rows;
        cngn::CsvReader reader(filename);

        while (std::optional<Row> row = reader.ReadLine()) {
            rows.push_back(std::move(row.value()));
        }

        return rows;
    }

    static void StringsToCsv(const std::string& filename, const std::vector<std::string>& lines) {
        std::ofstream ofs(filename);

        if (!ofs.is_open()) {
            throw std::runtime_error("Could not open file for writing");
        }

        for (const auto& line : lines) {
            ofs << line << '\n';
        }
    }

    static std::optional<Batch> BatchFromCsv(const std::string& filename, size_t batch_size = 1) {
        cngn::BatchedCsvReader reader(filename);

        return reader.ReadBatch(batch_size);
    }
};