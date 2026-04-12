#include "Scan.h"

namespace cngn {

Scan::Scan(const std::string& filename) : reader_(filename) {
}

void Scan::Open() {
}

std::optional<Batch> Scan::Next() {
    return reader_.ReadBatch(batch_num_++);
}

void Scan::Close() {
}

}  // namespace cngn