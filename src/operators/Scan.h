#pragma once

#include "../kernel/BatchedReader.h"
#include "Operator.h"

namespace cngn {
class Scan : public Operator {
public:
    explicit Scan(const std::string &filename, Schema need_columns_schema);

    void Open() override;
    std::optional<Batch> Next() override;
    void Close() override;

private:
    BatchedReader reader_;
    Schema schema_;
};
}  // namespace cngn