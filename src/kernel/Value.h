#pragma once

#include <stdexcept>
#include <string>
#include <variant>

#include "Types.h"

namespace cngn {
class Value {
public:
    using PhysTypeVariant = std::variant<PhysicalType<Type::Int64>, PhysicalType<Type::String>>;

    explicit Value(const PhysicalType<Type::Int64>& value) : value_(value) {
    }
    explicit Value(const PhysicalType<Type::String>& value) : value_(value) {
    }

    PhysTypeVariant GetValue() const {
        return value_;
    }

    std::string ToString() const {
        return std::visit(
            []<typename T>(const T& value) -> std::string {
                using NowType = std::decay_t<T>;
                if constexpr (std::is_same_v<NowType, PhysicalType<Type::Int64>>) {
                    return std::to_string(value);
                } else if constexpr (std::is_same_v<NowType, PhysicalType<Type::String>>) {
                    return value;
                } else {
                    throw std::invalid_argument("Unknown type");
                }
            },
            value_);
    }

private:
    PhysTypeVariant value_;
};
}  // namespace cngn