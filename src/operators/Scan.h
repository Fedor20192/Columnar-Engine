#pragma once

#include "../kernel/BatchedReader.h"
#include "Operator.h"

namespace cngn {
class Scan : public Operator {
public:
    explicit Scan(const std::string &filename);

    void Open() override;
    std::optional<Batch> Next() override;
    void Close() override;

private:
    BatchedReader reader_;
    size_t batch_num_{0};
};
}  // namespace cngn