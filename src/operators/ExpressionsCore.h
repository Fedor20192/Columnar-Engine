#pragma once

#include <regex>

#include "../kernel/Column.h"

namespace cngn {
namespace operators {

Column NotEqual(const Column &, const Column &);

Column Equal(const Column &, const Column &);

Column Gt(const Column &, const Column &);

Column Div(const Column &, const Column &);

Column ExtractMinuteFromCol(const Column &);

Column Contains(const Column &, const std::string &, bool);

Column StrLen(const Column &);

Column Regex(const Column &, const std::string &, const std::regex &);

Column And(const Column &, const Column &);

PhysTypeVariant Sum(const Column &);

std::optional<PhysTypeVariant> Min(const Column &);

std::optional<PhysTypeVariant> Max(const Column &);

}  // namespace operators
}  // namespace cngn