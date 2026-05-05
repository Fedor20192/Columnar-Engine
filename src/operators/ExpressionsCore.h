#pragma once

#include "../kernel/Column.h"

namespace cngn {
namespace operators {

Column NotEqual(const Column &a, const Column &b);

Column Div(const Column &a, const Column &b);

PhysTypeVariant Sum(const Column &a);

}
}  // namespace cngn