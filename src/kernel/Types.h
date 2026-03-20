#pragma once

#include <chrono>
#include <concepts>
#include <fstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace cngn {
enum class Type {
    Int64,
    Int32,
    Int16,
    String,
    Timestamp,
    Date,
};

template <Type>
struct PhysTypeWrapper {};

template <>
struct PhysTypeWrapper<Type::Int64> {
    using PhysicalType = int64_t;
};

template <>
struct PhysTypeWrapper<Type::Int32> {
    using PhysicalType = int32_t;
};

template <>
struct PhysTypeWrapper<Type::Int16> {
    using PhysicalType = int16_t;
};

template <>
struct PhysTypeWrapper<Type::String> {
    using PhysicalType = std::string;
};

template <>
struct PhysTypeWrapper<Type::Timestamp> {
    using PhysicalType = uint64_t;
};

template <>
struct PhysTypeWrapper<Type::Date> {
    using PhysicalType = uint32_t;
};

template <Type type>
using PhysicalType = PhysTypeWrapper<type>::PhysicalType;

using PhysTypeVariant = std::variant<PhysicalType<Type::Int64>, PhysicalType<Type::Int32>,
                                     PhysicalType<Type::Int16>, PhysicalType<Type::String>,
                                     PhysicalType<Type::Timestamp>, PhysicalType<Type::Date>>;

template <Type type>
using ArrayType = std::vector<PhysicalType<type>>;

using ArrayTypeVariant =
    std::variant<ArrayType<Type::Int64>, ArrayType<Type::Int32>, ArrayType<Type::Int16>,
                 ArrayType<Type::String>, ArrayType<Type::Timestamp>, ArrayType<Type::Date>>;

struct SerializeType {
    template <Type type>
    std::string operator()() const {
        if constexpr (type == Type::Int64) {
            return "int64";
        } else if constexpr (type == Type::Int32) {
            return "int32";
        } else if constexpr (type == Type::Int16) {
            return "int16";
        } else if constexpr (type == Type::String) {
            return "string";
        } else if constexpr (type == Type::Timestamp) {
            return "timestamp";
        } else if constexpr (type == Type::Date) {
            return "date";
        } else {
            throw std::runtime_error("Unknown type");
        }
    }
};

Type DeserializeType(const std::string &name);

std::chrono::system_clock::time_point ParseDatetime(const std::string &s, bool need_time);

template <Type type>
PhysicalType<type> Deserialize(const std::string &s) {
    constexpr std::chrono::system_clock::time_point kSinceEpoch{};
    using PT = PhysicalType<type>;
    if constexpr (type == Type::Timestamp) {
        auto tp = ParseDatetime(s, true);
        return PT(std::chrono::duration_cast<std::chrono::milliseconds>(tp - kSinceEpoch).count());
    } else if constexpr (type == Type::Date) {
        auto tp = ParseDatetime(s, false);
        return PT(std::chrono::duration_cast<std::chrono::days>(tp - kSinceEpoch).count());
    } else if constexpr (std::is_integral_v<PT>) {
        return PT(stoll(s));
    } else if constexpr (type == Type::String) {
        return s;
    } else {
        throw std::runtime_error("Unknown type");
    }
}

struct Reader {
    template <typename T>
    T operator()(std::ifstream &file) const {
        if constexpr (std::is_integral_v<T>) {
            T value;
            file.read(reinterpret_cast<char *>(&value), sizeof(T));
            return value;
        } else if constexpr (std::is_same_v<T, std::string>) {
            int64_t size = operator()<int64_t>(file);
            std::string value(size, 'a');
            file.read(value.data(), size);
            return value;
        } else {
            throw std::runtime_error("Unknown type");
        }
    }
};

template <typename T>
void Write(const T &value, std::ofstream &file);

template <std::integral T>
void Write(const T &value, std::ofstream &file) {
    file.write(reinterpret_cast<const char *>(&value), sizeof(value));
}

template <>
inline void Write(const std::string &value, std::ofstream &file) {
    Write(value.size(), file);
    file.write(value.data(), value.size());
}

std::string ToString(const PhysTypeVariant &x);

template <typename Callable, typename... Args>
auto DispatchOnType(Type type, Callable &&f, Args &&...args) {
    switch (type) {
        case Type::Int64:
            return std::forward<Callable>(f).template operator()<Type::Int64>(
                std::forward<Args>(args)...);
        case Type::Int32:
            return std::forward<Callable>(f).template operator()<Type::Int32>(
                std::forward<Args>(args)...);
        case Type::Int16:
            return std::forward<Callable>(f).template operator()<Type::Int16>(
                std::forward<Args>(args)...);
        case Type::String:
            return std::forward<Callable>(f).template operator()<Type::String>(
                std::forward<Args>(args)...);
        case Type::Timestamp:
            return std::forward<Callable>(f).template operator()<Type::Timestamp>(
                std::forward<Args>(args)...);
        case Type::Date:
            return std::forward<Callable>(f).template operator()<Type::Date>(
                std::forward<Args>(args)...);
        default:
            throw std::runtime_error("Unknown Type");
    }
}

template <typename Callable, typename... Args>
auto DispatchOnPhysType(Type type, Callable &&f, Args &&...args) {
    switch (type) {
        case Type::Int64:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::Int64>>(
                    std::forward<Args>(args)...));
        case Type::Int32:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::Int32>>(
                    std::forward<Args>(args)...));
        case Type::Int16:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::Int16>>(
                    std::forward<Args>(args)...));
        case Type::String:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::String>>(
                    std::forward<Args>(args)...));
        case Type::Timestamp:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::Timestamp>>(
                    std::forward<Args>(args)...));
        case Type::Date:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::Date>>(
                    std::forward<Args>(args)...));
        default:
            throw std::runtime_error("Unknown Type");
    }
}

}  // namespace cngn