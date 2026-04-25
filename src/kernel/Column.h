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
    std::shared_ptr<char[]> buffer_ptr_;
};
}  // namespace cngn