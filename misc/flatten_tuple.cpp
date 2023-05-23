#include <iostream>
#include <cstddef>
#include <tuple>
#include <type_traits>

template <typename... Ss>
auto ref_tup(std::tuple<Ss...>& vs) {
    return std::apply([](auto&... xs) { return std::tuple<Ss&...>(xs...); },
                      vs);
}

template <size_t Depth, size_t Max, typename... Ts>
auto flatten_tup(std::tuple<Ts...>& tup);

template <size_t Depth, size_t Max, typename T>
auto flatten_tup(T&);

template <size_t Depth, size_t Max, typename A>
auto flatten_h(A&& a) {
    if constexpr (Depth >= Max)
        return a;
    else
        return flatten_tup<Depth + 1, Max>(a);
}

template <size_t Depth, size_t Max, typename A, typename... Ts>
auto flatten_h(A&& a, Ts&&... vs) {
    if constexpr (Depth >= Max)
        return std::tuple<A&, Ts&...>(a, vs...);
    else
        return std::tuple_cat(flatten_tup<Depth + 1, Max>(a),
                              flatten_h<Depth, Max>(vs...));
}

template <size_t Depth, size_t Max, typename... Ts>
auto flatten_tup(std::tuple<Ts...>& tup) {
    if constexpr (Depth >= Max) {
        return ref_tup(tup);
    } else {
        return std::apply(
            [](auto&&... xs) { return flatten_h<Depth, Max>(xs...); }, tup);
    }
}

template <size_t Depth, size_t Max, typename T>
auto flatten_tup(T& v) {
    return std::tuple<T&>(v);
}

template <size_t N, typename... Ts>
auto flatten(std::tuple<Ts...>& tup) {
    return std::apply(
        [](auto&&... vs) {
            return flatten_h<0, N>(std::forward<decltype(vs)>(vs)...);
        },
        tup);
}

int main() {
    auto tup = std::tuple(1, std::tuple(2, 3), std::tuple(4, std::tuple(5)));
    auto&& [a, b, c, d, e] = flatten<1>(tup);

    std::cout << a << b << c << d << "\n";
    std::cout << std::is_same_v<decltype(e), std::tuple<int>&> << "\n";

    return 0;
}
