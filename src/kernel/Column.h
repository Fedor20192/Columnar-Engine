#pragma once

#include <variant>

#include "Types.h"
#include "Value.h"
namespace cngn {
class Column {
public:
    using ArrayTypeVariant = std::variant<ArrayType<Type::Int64>, ArrayType<Type::String>>;

    explicit Column(const ArrayType<Type::Int64>& array) : array_(array) {
    }

    explicit Column(const ArrayType<Type::String>& array) : array_(array) {
    }

    size_t Size() const {
        return std::visit([](const auto& arr) -> size_t { return arr.size(); }, array_);
    }

    Value operator[](size_t index) const {
        return std::visit([index](const auto& value) -> Value { return Value(value[index]); },
                          array_);
    }

private:
    ArrayTypeVariant array_;
};
}  // namespace cngn