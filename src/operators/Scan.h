#pragma once

#include "../kernel/BatchedReader.h"
#include "Operator.h"

namespace cngn {
class Scan : public Operator {
public:
    explicit Scan(const std::string &filename, const std::shared_ptr<Context> &);

    void Open() override;
    std::optional<Batch> Next() override;
    void Close() override;

private:
    BatchedReader reader_;
};
}  // namespace cngn