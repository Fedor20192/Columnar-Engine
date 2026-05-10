#pragma once

#include <variant>

#include "Types.h"

namespace cngn {
class Column {
public:
    template <typename T>
        requires std::is_constructible_v<ArrayTypeVariant, T>
    explicit Column(T&& array, const std::shared_ptr<char[]>& ptr = nullptr) noexcept
        : array_(std::forward<T>(array)), buffer_ptr_(ptr) {
    }

    explicit Column(Type type, size_t capacity) {
        auto init_array = [this, capacity]<Type type>() {
            std::vector<PhysicalType<type>> vec;
            vec.reserve(capacity);
            array_ = std::move(vec);
        };
        DispatchOnType(type, init_array);
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

    Type GetType() const {
        return std::visit(
            []<typename T0>(T0&&) -> Type {
                using T = std::decay_t<T0>;

                if constexpr (std::is_same_v<T, ArrayType<Type::Int128>>) {
                    return Type::Int128;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::UInt64>>) {
                    return Type::UInt64;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::Int64>>) {
                    return Type::Int64;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::Int32>>) {
                    return Type::Int32;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::Int16>>) {
                    return Type::Int16;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::Bool>>) {
                    return Type::Bool;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::String>>) {
                    return Type::String;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::MetaString>>) {
                    return Type::MetaString;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::Timestamp>>) {
                    return Type::Timestamp;
                } else if constexpr (std::is_same_v<T, ArrayType<Type::Date>>) {
                    return Type::Date;
                } else {
                    static_assert(false, "[Column::GetType:] unknown type");
                }
                return Type::Bool;
            },
            array_);  // todo: убрать это блядство
    }

    std::shared_ptr<char[]> GetOwningBuffer() const {
        return buffer_ptr_;
    }

    template <Type type>
    void PushBack(const PhysicalType<type>& value) {
        std::visit(
            [&]<typename T0>(T0&& arr) {
                using Vec = std::decay_t<T0>;
                using Elem = Vec::value_type;
                using Expected = PhysicalType<type>;

                if constexpr (std::is_same_v<Elem, Expected>) {
                    arr.push_back(value);
                } else {
                    throw std::runtime_error("[Column::PushBack:] type mismatch");
                }
            },
            array_);
    }

private:
    ArrayTypeVariant array_;
    std::shared_ptr<char[]> buffer_ptr_;
};
}  // namespace cngn