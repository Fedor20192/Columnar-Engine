#pragma once

#include <variant>

#include "Types.h"

namespace cngn {
class Column {
public:
    using ArrayTypeVariant = std::variant<ArrayType<Type::Int64>, ArrayType<Type::String>>;

    template <Type type>
    explicit Column(ArrayType<type> array) : array_(array) {
    }

    size_t Size() const {
        return std::visit([](const auto& arr) -> size_t { return arr.size(); }, array_);
    }

    Value operator[](size_t index) const {
        return std::visit([index]<Type type>(const ArrayType<type>& arr) { return Value(arr.at(index)); }, array_);
    }

private:
    ArrayTypeVariant array_;
};
}  // namespace cngn