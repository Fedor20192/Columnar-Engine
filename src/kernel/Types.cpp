#include "Types.h"

#include "glog/logging.h"

namespace cngn {

std::chrono::system_clock::time_point ParseDatetime(const std::string& s, bool need_time) {
    std::vector<unsigned> numbers;
    numbers.reserve(6);

    std::string now;
    for (char c : s) {
        if (c == ' ' || c == ':' || c == '-') {
            numbers.push_back(std::stoull(now));
            now.clear();
        } else {
            now += c;
        }
    }
    numbers.push_back(std::stoull(now));

    using std::chrono::hours, std::chrono::minutes, std::chrono::seconds;
    using std::chrono::year, std::chrono::month, std::chrono::day;
    using std::chrono::year_month_day, std::chrono::sys_days;

    if (numbers.size() < 3) {
        DLOG(ERROR) << "Too little date string: " << s << std::endl;
        throw std::runtime_error("Too little date string");
    }

    year_month_day ymd{year{static_cast<int>(numbers[0])}, month{numbers[1]}, day{numbers[2]}};
    auto time = hours{0} + minutes{0} + seconds{0};

    if (need_time) {
        if (numbers.size() < 6) {
            DLOG(ERROR) << "Too little datetime string: " << s << std::endl;
            throw std::runtime_error("Too little datetime string");
        }
        time = hours{numbers[3]} + minutes{numbers[4]} + seconds{numbers[5]};
    }

    return sys_days{ymd} + time;
}

Type DeserializeType(const std::string& name) {
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
    if (name == "timestamp") {
        return Type::Timestamp;
    }
    if (name == "date") {
        return Type::Date;
    }

    throw std::runtime_error("Unknown type: " + name);
}

std::string ToString(const PhysTypeVariant& x) {
    return std::visit(
        []<typename T>(const T& value) -> std::string {
            using NowType = std::decay_t<T>;
            if constexpr (std::is_same_v<NowType, PhysicalType<Type::Int64>> ||
                          std::is_same_v<NowType, PhysicalType<Type::Int32>> ||
                          std::is_same_v<NowType, PhysicalType<Type::Int16>>) {
                return std::to_string(value);
            } else if constexpr (std::is_same_v<NowType, PhysicalType<Type::String>>) {
                return value;
            } else {
                throw std::invalid_argument("Unknown type");
            }
        },
        x);
}

}  // namespace cngn
