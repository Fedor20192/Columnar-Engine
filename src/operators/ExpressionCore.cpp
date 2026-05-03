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

}  // namespace operators
}  // namespace cngn