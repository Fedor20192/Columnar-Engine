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
    Int128,
    UInt64,
    Int64,
    Int32,
    Int16,
    Bool,
    String,
    MetaString,
    Timestamp,
    Date,
};

struct Date {
    uint32_t days;
    auto operator<=>(const Date &rhs) const = default;
};

struct Timestamp {
    uint64_t seconds;
    auto operator<=>(const Timestamp &) const = default;
};

template <Type>
struct PhysTypeWrapper {};

template <>
struct PhysTypeWrapper<Type::Int128> {
    using PhysicalType = __int128_t;
};

template <>
struct PhysTypeWrapper<Type::UInt64> {
    using PhysicalType = uint64_t;
};

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
struct PhysTypeWrapper<Type::Bool> {
    using PhysicalType = char;
};

template <>
struct PhysTypeWrapper<Type::String> {
    using PhysicalType = std::string_view;
};

template <>
struct PhysTypeWrapper<Type::MetaString> {
    using PhysicalType = std::string;
};

template <>
struct PhysTypeWrapper<Type::Timestamp> {
    using PhysicalType = Timestamp;
};

template <>
struct PhysTypeWrapper<Type::Date> {
    using PhysicalType = Date;
};

template <Type type>
using PhysicalType = PhysTypeWrapper<type>::PhysicalType;

using PhysTypeVariant =
    std::variant<PhysicalType<Type::Int128>, PhysicalType<Type::UInt64>, PhysicalType<Type::Int64>,
                 PhysicalType<Type::Int32>, PhysicalType<Type::Int16>, PhysicalType<Type::Bool>,
                 PhysicalType<Type::String>, PhysicalType<Type::MetaString>,
                 PhysicalType<Type::Timestamp>, PhysicalType<Type::Date>>;

template <Type type>
using ArrayType = std::vector<PhysicalType<type>>;

using ArrayTypeVariant =
    std::variant<ArrayType<Type::Int128>, ArrayType<Type::UInt64>, ArrayType<Type::Int64>,
                 ArrayType<Type::Int32>, ArrayType<Type::Int16>, ArrayType<Type::Bool>,
                 ArrayType<Type::String>, ArrayType<Type::MetaString>, ArrayType<Type::Timestamp>,
                 ArrayType<Type::Date>>;

struct SerializeType {
    template <Type type>
    constexpr std::string operator()() const {
        if constexpr (type == Type::Int128) {
            return "int128";
        } else if constexpr (type == Type::UInt64) {
            return "uint64";
        } else if constexpr (type == Type::Int64) {
            return "int64";
        } else if constexpr (type == Type::Int32) {
            return "int32";
        } else if constexpr (type == Type::Int16) {
            return "int16";
        } else if constexpr (type == Type::Bool) {
            return "bool";
        } else if constexpr (type == Type::String) {
            return "string";
        } else if constexpr (type == Type::MetaString) {
            return "metastring";
        } else if constexpr (type == Type::Timestamp) {
            return "timestamp";
        } else if constexpr (type == Type::Date) {
            return "date";
        } else {
            static_assert(0, "[SerializeType]: Unknown type");
            return "";
        }
    }
};

Type DeserializeType(const std::string &name);

std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> ParseDatetime(
    std::string_view s, bool need_time);

template <Type type>
PhysicalType<type> Deserialize(std::string_view s) {
    using SysSeconds = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
    constexpr SysSeconds kSinceEpoch{};
    using PT = PhysicalType<type>;
    if constexpr (type == Type::Timestamp) {
        auto tp = ParseDatetime(s, true);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(tp - kSinceEpoch).count();
        return PT(seconds);
    } else if constexpr (type == Type::Date) {
        auto tp = ParseDatetime(s, false);
        auto days = std::chrono::duration_cast<std::chrono::days>(tp - kSinceEpoch).count();
        return PT(days);
    } else if constexpr (std::is_integral_v<PT>) {
        __int128_t val = 0;
        for (size_t pos = s[0] == '-'; pos < s.size(); ++pos) {
            val = val * 10 + s[pos] - '0';
        }
        if (s[0] == '-') {
            val *= -1;
        }
        return PT(val);
    } else if constexpr (type == Type::String) {
        return s;  // todo: Опасное место, надо подумать, как запретить доступ к Deserilize без
                   // мгновенного использования
    } else if constexpr (type == Type::MetaString) {
        return std::string(s);
    } else {
        static_assert(false, "[Deserialize]: Unknown type");
    }
}

struct Reader {
    template <typename T>
    static constexpr bool IsIntegral() {
        if constexpr (std::is_integral_v<T> || std::is_same_v<T, Date> ||
                      std::is_same_v<T, Timestamp>) {
            return true;
        }
        return false;
    }

    template <typename T>
    T operator()(std::ifstream &file) const {
        if constexpr (IsIntegral<T>()) {
            T value;
            file.read(reinterpret_cast<char *>(&value), sizeof(T));
            return value;
        } else {
            uint32_t size = operator()<uint32_t>(file);
            std::string value(size, 'a');
            file.read(value.data(), size);
            return value;
        }
    }

    template <typename T>
    std::vector<T> operator()(std::ifstream &file, uint32_t rows_cnt,
                              std::shared_ptr<char[]> &buf_ptr) {
        if constexpr (IsIntegral<T>()) {
            std::vector<T> ans(rows_cnt);
            file.read(reinterpret_cast<char *>(ans.data()), ans.size() * sizeof(T));
            buf_ptr = nullptr;
            return ans;
        } else {
            std::vector<uint32_t> offsets(rows_cnt + 1);
            file.read(reinterpret_cast<char *>(offsets.data()), offsets.size() * sizeof(uint32_t));

            uint32_t size = offsets.back() - offsets[0];
            std::vector<T> ans;
            ans.reserve(rows_cnt);

            buf_ptr = std::make_shared<char[]>(size);
            file.read(buf_ptr.get(), size);

            for (uint32_t i = 0; i < rows_cnt; i++) {
                uint32_t sz = offsets[i + 1] - offsets[i];
                ans.emplace_back(buf_ptr.get() + (offsets[i] - offsets[0]), sz);
            }

            if constexpr (std::is_same_v<T, std::string>) {
                buf_ptr = nullptr;
            }

            return ans;
        }
    }
};

template <typename T>
    requires std::integral<T> || std::is_same_v<T, Date> || std::is_same_v<T, Timestamp>
void Write(const T &value, std::ofstream &file) {
    file.write(reinterpret_cast<const char *>(&value), sizeof(value));
}

void Write(std::string_view value, std::ofstream &file);

template <typename T>
    requires std::integral<T> || std::is_same_v<T, Date> || std::is_same_v<T, Timestamp>
void Write(const std::vector<T> &value, std::ofstream &file) {
    file.write(reinterpret_cast<const char *>(value.data()), sizeof(T) * value.size());
}

template <typename T>
    requires std::is_convertible_v<T, std::string_view>
void Write(const std::vector<T> &value, std::ofstream &file) {
    size_t buffer_size = (value.size() + 1) * sizeof(uint32_t);
    for (const auto &str : value) {
        buffer_size += str.size() * sizeof(char);
    }

    std::vector<char> buffer;
    buffer.reserve(buffer_size);

    uint32_t su = 0;
    for (const auto &str : value) {
        auto bytes = reinterpret_cast<const char *>(&su);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(uint32_t));
        su += str.size();
    }
    auto bytes = reinterpret_cast<const char *>(&su);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(uint32_t));

    for (const auto &str : value) {
        buffer.insert(buffer.end(), str.begin(), str.end());  // todo: поменять на memcpy
    }

    file.write(buffer.data(), buffer_size);
}

std::string ToString(const PhysTypeVariant &x);

template <typename Callable, typename... Args>
auto DispatchOnType(Type type, Callable &&f, Args &&...args) {
    switch (type) {
        case Type::Int128:
            return std::forward<Callable>(f).template operator()<Type::Int128>(
                std::forward<Args>(args)...);
        case Type::UInt64:
            return std::forward<Callable>(f).template operator()<Type::UInt64>(
                std::forward<Args>(args)...);
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
        case Type::MetaString:
            return std::forward<Callable>(f).template operator()<Type::MetaString>(
                std::forward<Args>(args)...);
        case Type::Timestamp:
            return std::forward<Callable>(f).template operator()<Type::Timestamp>(
                std::forward<Args>(args)...);
        case Type::Date:
            return std::forward<Callable>(f).template operator()<Type::Date>(
                std::forward<Args>(args)...);
        default:
            throw std::runtime_error("[Dispatch on type]: unknown Type");
    }
}

}  // namespace cngn