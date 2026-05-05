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
static ArrayType<type_1> Divv(const ArrayType<type_1> &a, const ArrayType<type_2> &b) {
    if (a.size() != b.size()) {
        throw std::logic_error("[Div]: different sizes");
    }

    ArrayType<type_1> ans(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        if (b[i] == 0) {
            throw std::logic_error("[Div]: division by zero");
        }
        ans[i] = a[i] / static_cast<PhysicalType<type_1>>(b[i]);
    }

    return ans;
}

Column NotEqual(const Column &a, const Column &b) {
    const auto type = a.GetType();

    auto bool_vec = DispatchOnType(type, [&]<Type type>() -> ArrayType<Type::Bool> {
        return Compare<type, std::not_equal_to<PhysicalType<type>>>(
            std::get<ArrayType<type>>(a.GetData()), std::get<ArrayType<type>>(b.GetData()));
    });

    return Column(std::move(bool_vec));
}

Column Div(const Column &a, const Column &b) {
    const auto type_a = a.GetType();

    return DispatchOnType(type_a, [&a, &b]<Type type_a>() -> Column {
        const auto &arr_a = std::get<ArrayType<type_a>>(a.GetData());

        const auto type_b = b.GetType();
        return DispatchOnType(type_b, [&arr_a, &b]<Type type_b>() -> Column {
            const auto &arr_b = std::get<ArrayType<type_b>>(b.GetData());
            if constexpr (IsArithmetic<type_a> && IsArithmetic<type_b>) {
                return Column(Divv<type_a, type_b>(arr_a, arr_b));
            }
            throw std::invalid_argument("[Div]: You are dividing non arithmetic objects");
        });

    });
}

PhysTypeVariant Sum(const Column &a) {
    Type type = a.GetType();
    if (type == Type::Bool || type == Type::Timestamp || type == Type::Date ||
        type == Type::String || type == Type::MetaString) {
        throw std::invalid_argument(
            "[Sum]: WTF? You are trying to sum ");  // todo: написать тип в выбросе
    }

    return DispatchOnType(type, [&]<Type type>() -> __int128_t {
        if constexpr (IsArithmetic<type>) {
            __int128_t sum = 0;
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