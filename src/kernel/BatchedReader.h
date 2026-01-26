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

    std::ifstream file_;
    Metadata metadata_;
};
}  // namespace cngn
