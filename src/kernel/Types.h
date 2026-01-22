#pragma once

#include <stdexcept>
#include <string>
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
using PhysicalType = PhysTypeWrapper<type>;

template <Type type>
using ArrayType = std::vector<PhysicalType<type>>;

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

}  // namespace cngn