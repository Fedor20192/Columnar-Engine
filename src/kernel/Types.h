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

inline std::string SerializeType(Type type) {
    switch (type) {
        case Type::Int64:
            return "int64";
        case Type::String:
            return "string";
        default:
            throw std::runtime_error("Serializing unknown type");
    }
}

inline Type DeserializeType(const std::string &name) {
    if (name == "int64") {
        return Type::Int64;
    } else if (name == "string") {
        return Type::String;
    } else {
        throw std::runtime_error("Unknown type: " + name);
    }
}

template <typename T>
T Read(std::ifstream &file);

template <std::integral T>
T Read(std::ifstream &file) {
    T value;
    file.read(reinterpret_cast<char *>(&value), sizeof(T));
    return value;
}

template <>
inline std::string Read(std::ifstream &file) {
    int64_t size = Read<int64_t>(file);

    std::string value(size, 'a');

    file.read(value.data(), size);

    return value;
}

template <Type T>
ArrayType<T> ReadColumnOfType(std::ifstream &in, int64_t rows_cnt) {
    ArrayType<T> column;
    column.reserve(rows_cnt);
    for (int64_t i = 0; i < rows_cnt; ++i) {
        column.push_back(Read<PhysicalType<T>>(in));
    }
    return column;
}

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

}  // namespace cngn