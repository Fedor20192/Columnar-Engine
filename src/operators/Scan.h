#pragma once

#include "../kernel/BatchedReader.h"
#include "Operator.h"

namespace cngn {
namespace operators {
class Scan : public Operator {
public:
    explicit Scan(const std::string &filename, Schema need_columns_schema);

    void Open() override;
    std::optional<std::shared_ptr<Batch>> Next() override;
    void Close() override;

private:
    BatchedReader reader_;
    Schema schema_;
};
}  // namespace operators
}  // namespace cngn