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

    template <Type type>
    void PushBack(const PhysicalType<type>& value) {
        std::visit(
            [&](auto& arr) {
                using Vec = std::decay_t<decltype(arr)>;
                using Elem = Vec::value_type;
                using Expected = PhysicalType<type>;

                if constexpr (std::is_same_v<Elem, Expected>) {
                    arr.push_back(value);
                }
            },
            array_);
    }

private:
    ArrayTypeVariant array_;
    std::shared_ptr<char[]> buffer_ptr_;
};
}  // namespace cngn