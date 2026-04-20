#include "Types.h"

#include <format>

#include "glog/logging.h"

namespace cngn {

using SysSeconds = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

SysSeconds ParseDatetime(const std::string &s, bool need_time) {
    unsigned numbers[6];

    unsigned now = 0;
    size_t pos = 0;
    for (char c : s) {
        if (c == ' ' || c == ':' || c == '-') {
            numbers[pos++] = now;
            now = 0;
        } else {
            now = now * 10 + c - '0';
        }
    }
    numbers[pos++] = now;

    using std::chrono::hours, std::chrono::minutes, std::chrono::seconds;
    using std::chrono::year, std::chrono::month, std::chrono::day;
    using std::chrono::year_month_day, std::chrono::sys_days;

    if (pos < 3) {
        DLOG(ERROR) << "Too little date string: " << s << std::endl;
        throw std::runtime_error("Too little date string");
    }

    year_month_day ymd{year{static_cast<int>(numbers[0])}, month{numbers[1]}, day{numbers[2]}};
    auto time = hours{0} + minutes{0} + seconds{0};

    if (need_time) {
        if (pos > 3) {
            time = hours{numbers[3]};
        }
        if (pos > 4) {
            time += minutes{numbers[4]};
        }
        if (pos > 5) {
            time += seconds{numbers[5]};
        }
    }

    SysSeconds ans = sys_days{ymd} + time;
    return ans;
}

Type DeserializeType(const std::string &name) {
    if (name == "uint64") {
        return Type::UInt64;
    }
    if (name == "int64") {
        return Type::Int64;
    }
    if (name == "int32") {
        return Type::Int32;
    }
    if (name == "int16") {
        return Type::Int16;
    }
    if (name == "string") {
        return Type::String;
    }
    if (name == "metastring") {
        return Type::MetaString;
    }
    if (name == "timestamp") {
        return Type::Timestamp;
    }
    if (name == "date") {
        return Type::Date;
    }

    throw std::runtime_error("Unknown type: " + name);
}

void Write(std::string_view value, std::ofstream &file) {
    Write(static_cast<uint32_t>(value.size()), file);
    file.write(value.data(), value.size());
}

std::string ToString(const PhysTypeVariant &x) {
    using std::chrono::seconds, std::chrono::year, std::chrono::days;
    using std::chrono::sys_seconds, std::chrono::sys_days;

    return std::visit(
        []<typename T>(const T &value) -> std::string {
            using NowType = std::decay_t<T>;
            if constexpr (std::is_same_v<NowType, PhysicalType<Type::UInt64>> ||
                          std::is_same_v<NowType, PhysicalType<Type::Int64>> ||
                          std::is_same_v<NowType, PhysicalType<Type::Int32>> ||
                          std::is_same_v<NowType, PhysicalType<Type::Int16>>) {
                return std::to_string(value);
            } else if constexpr (std::is_same_v<NowType, PhysicalType<Type::Date>>) {
                auto current_date = sys_days{} + days{value.days};
                return std::format("{:%Y-%m-%d}", current_date);
            } else if constexpr (std::is_same_v<NowType, PhysicalType<Type::Timestamp>>) {
                auto current_time = sys_seconds{} + seconds{value.seconds};
                return std::format("{:%Y-%m-%d %H:%M:%S}", current_time);
            } else if constexpr (std::is_same_v<NowType, PhysicalType<Type::String>> ||
                                 std::is_same_v<NowType, PhysicalType<Type::MetaString>>) {
                return std::string(value);
            } else {
                throw std::invalid_argument("ToString unknown type");
            }
        },
        x);
}

}  // namespace cngn
