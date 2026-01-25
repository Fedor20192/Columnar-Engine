#pragma once

#include <fstream>
#include <optional>

#include "Batch.h"
#include "Metadata.h"

namespace cngn {

class BatchedReader {
public:
    explicit BatchedReader(const std::string& filename);

    std::optional<Batch> ReadBatch(size_t num_of_batch);

    const Metadata& GetMetadata() const;

private:
    static Schema ReadSchema(std::ifstream& in);
    static Metadata ReadMetadata(std::ifstream& in);

    PhysTypeVariant ReadElem(Type type);

    template <Type type>
    Column ReadColumn(int64_t rows_cnt) {
        ArrayType<type> array;
        array.reserve(rows_cnt);

        for (int64_t i = 0; i < rows_cnt; i++) {
            array.emplace_back(Read<PhysicalType<type>>(file_));
        }
        return Column(std::move(array));
    }

    std::ifstream file_;
    Metadata metadata_;
};
}  // namespace cngn
