#pragma once

#include "../kernel/Column.h"

namespace cngn {
namespace operators {

Column NotEqual(const Column &a, const Column &b);

__int128_t Sum(const Column &a);

}
}  // namespace cngn