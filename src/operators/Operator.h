#pragma once

#include <optional>

#include "../kernel/Batch.h"
namespace cngn {
class Operator {
public:
    virtual void Open() = 0;
    virtual std::optional<Batch> Next() = 0;
    virtual void Close() = 0;

    virtual ~Operator() = default;

protected:
    Operator() = default;
};
}  // namespace cngn