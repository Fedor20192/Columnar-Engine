#include "Scan.h"

namespace cngn {

Scan::Scan(const std::string& filename) : reader_(filename) {
}

Scan::Scan(const std::string& filename, const std::vector<std::string>& columns_names)
    : reader_(filename) {
    columns_names_ = columns_names;
}

void Scan::Open() {
}

std::optional<Batch> Scan::Next() {
    return reader_.ReadBatch();
}

void Scan::Close() {
}

}  // namespace cngn