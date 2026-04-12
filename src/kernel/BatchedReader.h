#pragma once

#include <fstream>
#include <optional>

#include "Batch.h"
#include "Metadata.h"

namespace cngn {

class BatchedReader {
public:
    explicit BatchedReader(const std::string& filename);

    std::optional<Batch> ReadBatch();

    const Metadata& GetMetadata() const;

    void SetIndices(std::vector<uint64_t>&& column_indices);

private:
    static Schema ReadSchema(std::ifstream& in);
    static Metadata ReadMetadata(std::ifstream& in);

    PhysTypeVariant ReadElem(Type type);

    std::ifstream file_;
    Metadata metadata_;
    uint64_t num_of_batch_{0};
    std::vector<uint64_t> column_indices_;
};
}  // namespace cngn
