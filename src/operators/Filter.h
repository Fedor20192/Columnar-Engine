#pragma once
#include "Operator.h"

namespace cngn {
class Filter : public Operator {
public:
    Filter(std::shared_ptr<Operator> child, std::shared_ptr<Operator> predicate);
};
}  // namespace cngn