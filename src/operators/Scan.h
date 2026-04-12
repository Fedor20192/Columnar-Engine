#pragma once

#include "../kernel/BatchedReader.h"
#include "Operator.h"

namespace cngn {
class Scan : public Operator {
public:
    explicit Scan(const std::string &filename);
    Scan(const std::string &filename, const std::vector<std::string> &columns_names);

    void Open() override;
    std::optional<Batch> Next() override;
    void Close() override;

private:
    BatchedReader reader_;
    size_t batch_num_{0};
    std::optional<std::vector<std::string>> columns_names_{std::nullopt};
};
}  // namespace cngn