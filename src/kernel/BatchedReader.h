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

    struct ReadColumn {
        template <Type type>
        Column operator()(int64_t rows_cnt, std::ifstream& in) {
            ArrayType<type> array;
            array.reserve(rows_cnt);

            for (int64_t i = 0; i < rows_cnt; i++) {
                array.emplace_back(Reader().operator()<PhysicalType<type>>(in));
            }
            return Column(std::move(array));
        }
    };

    std::ifstream file_;
    Metadata metadata_;
};
}  // namespace cngn
