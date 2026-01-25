#pragma once

#include <concepts>
#include <fstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace cngn {
enum class Type {
    Int64,
    String,
};

template <Type>
struct PhysTypeWrapper {};

template <>
struct PhysTypeWrapper<Type::Int64> {
    using PhysicalType = int64_t;
};

template <>
struct PhysTypeWrapper<Type::String> {
    using PhysicalType = std::string;
};

template <Type type>
using PhysicalType = PhysTypeWrapper<type>::PhysicalType;

using PhysTypeVariant = std::variant<PhysicalType<Type::Int64>, PhysicalType<Type::String>>;

template <Type type>
using ArrayType = std::vector<PhysicalType<type>>;

using ArrayTypeVariant = std::variant<ArrayType<Type::Int64>, ArrayType<Type::String>>;

struct SerializeType {
    template <Type type>
    std::string operator()() const {
        if constexpr (type == Type::Int64) {
            return "int64";
        } else if constexpr (type == Type::String) {
            return "string";
        } else {
            throw std::runtime_error("Unknown type");
        }
    }
};

inline Type DeserializeType(const std::string &name) {
    if (name == "int64") {
        return Type::Int64;
    } else if (name == "string") {
        return Type::String;
    } else {
        throw std::runtime_error("Unknown type: " + name);
    }
}

template <Type type>
PhysicalType<type> Deserialize(const std::string &s) {
    if constexpr (std::is_integral_v<PhysicalType<type>>) {
        return PhysicalType<type>(stoll(s));
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

template <typename Callable, typename... Args>
auto DispatchOnType(Type type, Callable &&f, Args &&...args) {
    switch (type) {
        case Type::Int64:
            return std::forward<Callable>(f).template operator()<Type::Int64>(
                std::forward<Args>(args)...);
        case Type::String:
            return std::forward<Callable>(f).template operator()<Type::String>(
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
        case Type::String:
            return PhysTypeVariant(
                std::forward<Callable>(f).template operator()<PhysicalType<Type::String>>(
                    std::forward<Args>(args)...));
        default:
            throw std::runtime_error("Unknown Type");
    }
}

}  // namespace cngn