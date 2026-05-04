#pragma once

#include <memory>
#include <optional>

#include "../kernel/Batch.h"

namespace cngn {
namespace operators {
class Operator {
public:
    virtual void Open() = 0;
    virtual std::optional<std::shared_ptr<Batch>> Next() = 0;
    virtual void Close() = 0;

    virtual ~Operator() = default;

protected:
    Operator() = default;
    Operator(const Operator&) = delete;
    Operator(Operator&&) = delete;
    Operator& operator=(const Operator&) = delete;
    Operator& operator=(Operator&&) = delete;
};
}  // namespace operators
}  // namespace cngn