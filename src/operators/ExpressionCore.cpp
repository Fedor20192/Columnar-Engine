#include "ExpressionsCore.h"

namespace cngn {
namespace operators {

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

Column NotEqual(const Column &a, const Column &b) {
    const auto type = a.GetType();

    auto bool_vec = DispatchOnType(type, [&]<Type type>() -> ArrayType<Type::Bool> {
        return Compare<type, std::not_equal_to<PhysicalType<type>>>(
            std::get<ArrayType<type>>(a.GetData()), std::get<ArrayType<type>>(b.GetData()));
    });

    return Column(std::move(bool_vec));
}

__int128_t Sum(const Column &a) {
    Type type = a.GetType();
    if (type == Type::Bool || type == Type::Timestamp || type == Type::Date ||
        type == Type::String || type == Type::MetaString) {
        throw std::invalid_argument(
            "[Sum]: WTF? You are trying to sum ");  // todo: написать тип в выбросе
    }

    return DispatchOnType(type, [&]<Type type>() -> __int128_t {
        if constexpr (type != Type::Timestamp && type != Type::Date &&
                      type != Type::MetaString && type != Type::String && type != Type::Bool) {
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