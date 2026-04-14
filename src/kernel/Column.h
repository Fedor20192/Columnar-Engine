#pragma once

#include <variant>

#include "Types.h"

namespace cngn {
class Column {
public:
    using OwningPtr = std::shared_ptr<std::vector<char>>; //todo: поменять на просто указатель на char[]
    explicit Column(ArrayType<Type::UInt64>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::Int64>&& array, const OwningPtr &ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::Int32>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::Int16>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::String>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::MetaString>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::Date>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }
    explicit Column(ArrayType<Type::Timestamp>&& array, const OwningPtr& ptr = nullptr) noexcept
        : array_(std::move(array)), buffer_ptr_(ptr) {
    }

    size_t Size() const {
        return std::visit([](const auto& arr) { return arr.size(); }, array_);
    }

    PhysTypeVariant operator[](size_t index) const {
        return std::visit([index](const auto& value) { return PhysTypeVariant(value[index]); },
                          array_);
    }

    bool operator==(const Column& other) const {
        return array_ == other.array_;
    }

    const ArrayTypeVariant& GetData() const {
        return array_;
    }

private:
    ArrayTypeVariant array_;
    OwningPtr buffer_ptr_;
};
}  // namespace cngn