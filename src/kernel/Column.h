#pragma once

#include <variant>

#include "Types.h"
#include "Value.h"

namespace cngn {
class Column {
public:
    explicit Column(ArrayType<Type::Int64> array) : array_(std::move(array)) {
    }

    explicit Column(ArrayType<Type::String> array) : array_(std::move(array)) {
    }

    size_t Size() const {
        return std::visit([](const auto& arr) { return arr.size(); }, array_);
    }

    Value operator[](size_t index) const {
        return std::visit([index](const auto& value) { return Value(value[index]); }, array_);
    }

    bool operator==(const Column&) const = default;

private:
    ArrayTypeVariant array_;
};
}  // namespace cngn