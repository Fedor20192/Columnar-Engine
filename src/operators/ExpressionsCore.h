#pragma once

#include "../kernel/Column.h"

namespace cngn {
namespace operators {

Column NotEqual(const Column &, const Column &);

Column Equal(const Column &, const Column &);

Column Div(const Column &, const Column &);

Column ExtractMinuteFromCol(const Column &);

Column Contains(const Column &, const std::string &);

Column And(const Column &, const Column &);

PhysTypeVariant Sum(const Column &);

std::optional<PhysTypeVariant> Min(const Column&);

std::optional<PhysTypeVariant> Max(const Column&);

}
}  // namespace cngn