#pragma once

#include <variant>

#include "Types.h"

namespace cngn {
class Column {
public:
    explicit Column(ArrayType<Type::Int64>&& array) noexcept : array_(std::move(array)) {
    }
    explicit Column(ArrayType<Type::Int32>&& array) noexcept : array_(std::move(array)) {
    }
    explicit Column(ArrayType<Type::Int16>&& array) noexcept : array_(std::move(array)) {
    }
    explicit Column(ArrayType<Type::String>&& array) noexcept : array_(std::move(array)) {
    }
    explicit Column(ArrayType<Type::Date>&& array) noexcept : array_(std::move(array)) {
    }
    explicit Column(ArrayType<Type::Timestamp>&& array) noexcept : array_(std::move(array)) {
    }

    size_t Size() const {
        return std::visit([](const auto& arr) { return arr.size(); }, array_);
    }

    PhysTypeVariant operator[](size_t index) const {
        return std::visit([index](const auto& value) { return PhysTypeVariant(value[index]); },
                          array_);
    }

    bool operator==(const Column&) const = default;

    const ArrayTypeVariant& GetData() const {
        return array_;
    }

private:
    ArrayTypeVariant array_;
};
}  // namespace cngn