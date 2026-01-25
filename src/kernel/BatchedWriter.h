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
    int64_t WriteElem(const Value& value) {
        std::visit([this](const auto& to_print) { Write(to_print, file_); }, value.GetValue());
        return file_.tellp();
    }

    std::ofstream file_;
    Metadata metadata_;
};
}  // namespace cngn
