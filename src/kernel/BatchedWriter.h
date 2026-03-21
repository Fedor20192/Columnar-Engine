#pragma once

#include <fstream>
#include <string>

#include "Batch.h"
#include "Metadata.h"

namespace cngn {
class BatchedWriter {
public:
    explicit BatchedWriter(const std::string& filename, const Schema& schema);

    void WriteBatch(const Batch& batch);

    void WriteMetadata();

    void Flush();

private:
    size_t WriteElem(const PhysTypeVariant& value);
    size_t WriteElem(const ArrayTypeVariant& value);

    std::ofstream file_;
    Metadata metadata_;
};
}  // namespace cngn
