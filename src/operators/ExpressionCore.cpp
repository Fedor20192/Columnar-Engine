#include <glog/logging.h>

#include <functional>

#include "ExpressionsCore.h"

namespace cngn {
namespace operators {

template <Type type>
concept IsArithmetic = type != Type::Timestamp && type != Type::Date && type != Type::MetaString &&
                       type != Type::String && type != Type::Bool;

template <Type type, typename Comparator>
static ArrayType<Type::Bool> Compare(const ArrayType<type> &l, const ArrayType<type> &r) {
    if (l.size() != r.size()) {
        throw std::logic_error("[Compare]: different sizes");
    }

    ArrayType<Type::Bool> ans(l.size());

    for (size_t i = 0; i < l.size(); i++) {
        ans[i] = Comparator{}(l[i], r[i]);
    }

    return ans;
}

template <Type type_1, Type type_2>
    requires IsArithmetic<type_1> && IsArithmetic<type_2>
static ArrayType<type_1> Divv(
    const ArrayType<type_1> &a, const ArrayType<type_2> &b,
    std::function<PhysicalType<type_1>(PhysicalType<type_1>, PhysicalType<type_2>)> alu) {
    if (a.size() != b.size()) {
        throw std::logic_error("[Div]: different sizes");
    }

    ArrayType<type_1> ans(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        ans[i] = alu(a[i], b[i]);
    }

    return ans;
}

template <Type type, typename Comparator>
static std::optional<PhysicalType<type>> MinMax(const ArrayType<type> &arr) {
    if (arr.empty()) {
        return std::nullopt;
    }

    Comparator cmp;

    auto ans = arr[0];

    for (size_t i = 1; i < arr.size(); i++) {
        if (cmp(arr[i], ans)) {
            ans = arr[i];
        }
    }

    return ans;
}

std::optional<PhysTypeVariant> Min(const Column &a) {
    return DispatchOnType(a.GetType(), [&]<Type type>() -> std::optional<PhysTypeVariant> {
        const auto &arr = std::get<ArrayType<type>>(a.GetData());
        return MinMax<type, std::less<PhysicalType<type>>>(arr);
    });
}

std::optional<PhysTypeVariant> Max(const Column &a) {
    return DispatchOnType(a.GetType(), [&]<Type type>() -> std::optional<PhysTypeVariant> {
        const auto &arr = std::get<ArrayType<type>>(a.GetData());
        return MinMax<type, std::greater<PhysicalType<type>>>(arr);
    });
}

Column NotEqual(const Column &a, const Column &b) {
    const auto type = a.GetType();

    auto bool_vec = DispatchOnType(type, [&]<Type type>() -> ArrayType<Type::Bool> {
        return Compare<type, std::not_equal_to<PhysicalType<type>>>(
            std::get<ArrayType<type>>(a.GetData()), std::get<ArrayType<type>>(b.GetData()));
    });

    return Column(std::move(bool_vec));
}

Column Equal(const Column &a, const Column &b) {
    const auto type = a.GetType();

    return Column(DispatchOnType(type, [&]<Type type>() -> ArrayType<Type::Bool> {
        return Compare<type, std::equal_to<PhysicalType<type>>>(
            std::get<ArrayType<type>>(a.GetData()), std::get<ArrayType<type>>(b.GetData()));
    }));
}

Column Div(const Column &a, const Column &b) {
    const auto type_a = a.GetType();

    return DispatchOnType(type_a, [&a, &b]<Type type_a>() -> Column {
        const auto &arr_a = std::get<ArrayType<type_a>>(a.GetData());

        const auto type_b = b.GetType();
        return DispatchOnType(type_b, [&arr_a, &b]<Type type_b>() -> Column {
            const auto &arr_b = std::get<ArrayType<type_b>>(b.GetData());
            if constexpr (IsArithmetic<type_a> && IsArithmetic<type_b>) {
                auto arithmetic = [](auto a, auto b) {
                    if (b == 0) {
                        throw std::logic_error("[Div]: division by zero");
                    }
                    return a / static_cast<decltype(a)>(b);
                };
                return Column(Divv<type_a, type_b>(arr_a, arr_b, std::move(arithmetic)));
            }
            throw std::invalid_argument("[Div]: You are dividing non arithmetic objects");
        });
    });
}

Column ExtractMinuteFromCol(const Column &a) {
    DLOG(INFO) << "[ExtractMinuteFromCol]: Started...\n";

    return DispatchOnType(a.GetType(), [&]<Type type>() -> Column {
        const auto &arr = std::get<ArrayType<type>>(a.GetData());
        ArrayType<Type::Int64> ans(arr.size());
        if constexpr (std::is_same_v<PhysicalType<type>, PhysicalType<Type::Timestamp>>) {
            for (size_t i = 0; i < arr.size(); i++) {
                ans[i] = (arr[i].seconds / 60) % 60;
            }

        } else {
            throw std::invalid_argument(
                "[ExtractMinuteFromCol]: You are trying to extract minute from non timestamp");
        }

        return Column(std::move(ans));
    });

    DLOG(INFO) << "[ExtractMinuteFromCol]: Finished\n";
}

Column Contains(const Column &a, const std::string &substr) {
    const Type type = a.GetType();

    if (type != Type::String && type != Type::MetaString) {
        throw std::invalid_argument("[Contains]: type must be string or metastring");
    }

    return DispatchOnType(type, [&]<Type type>() -> Column {
        if constexpr (type == Type::String || type == Type::MetaString) {
            ArrayType<Type::Bool> ans(a.Size());
            const auto &arr = std::get<ArrayType<type>>(a.GetData());

            for (size_t i = 0; i < a.Size(); i++) {
                ans[i] = arr[i].find(substr) != std::string::npos;
            }

            return Column(std::move(ans));
        }

        throw std::invalid_argument("[Contains]: type must be string or metastring");
    });
}

Column And(const Column &a, const Column &b) {
    const auto type = a.GetType();

    return DispatchOnType(type, [&]<Type type>() -> Column {
        if constexpr (type == Type::Bool) {
            ArrayType<Type::Bool> ans(a.Size());
            const auto &arr_a = std::get<ArrayType<type>>(a.GetData());
            const auto &arr_b = std::get<ArrayType<type>>(b.GetData());

            for (size_t i = 0; i < a.Size(); i++) {
                ans[i] = arr_a[i] && arr_b[i];
            }

            return Column(std::move(ans));
        }

        throw std::invalid_argument("[And]: You are trying to use non bool objects");
    });
}

PhysTypeVariant Sum(const Column &a) {
    Type type = a.GetType();
    if (type == Type::Bool || type == Type::Timestamp || type == Type::Date ||
        type == Type::String || type == Type::MetaString) {
        throw std::invalid_argument(
            "[Sum]: WTF? You are trying to sum ");  // todo: написать тип в выбросе
    }

    return DispatchOnType(type, [&]<Type type>() -> PhysicalType<Type::Int128> {
        if constexpr (IsArithmetic<type>) {
            PhysicalType<Type::Int128> sum = 0;
            const auto &arr = std::get<ArrayType<type>>(a.GetData());

            for (size_t i = 0; i < arr.size(); i++) {
                sum += arr[i];
            }

            return sum;
        }

        throw std::invalid_argument("[Sum]: you are sneaky bastard");
    });
}

}  // namespace operators
}  // namespace cngn