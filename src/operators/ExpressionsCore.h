#pragma once

#include <regex>

#include "../kernel/Column.h"

namespace cngn {
namespace operators {

template <Type type>
concept IsArithmetic = type != Type::Timestamp && type != Type::Date && type != Type::MetaString &&
                       type != Type::String && type != Type::Bool;

Column NotEqual(const Column &, const Column &);

Column Equal(const Column &, const Column &);

Column Gt(const Column &, const Column &);

Column Geq(const Column &, const Column &);

Column Add(const Column &, const Column &);

Column Mul(const Column &, const Column &);

Column Div(const Column &, const Column &);

Column ExtractMinuteFromCol(const Column &);

Column Contains(const Column &, const std::string &, bool);

Column StrLen(const Column &);

Column Regex(const Column &, const std::string &, const std::regex &);

Column And(const Column &, const Column &);

Column Or(const Column &, const Column &);

Column Case(const Column &, const Column &, const Column &);

PhysTypeVariant Sum(const Column &);

}  // namespace operators
}  // namespace cngn