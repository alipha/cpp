#ifndef LIPH_COMPILES_HPP
#define LIPH_COMPILES_HPP

#include <type_traits>
#include <utility>

#define LIPH_WRAP(...) __VA_ARGS__
#define LIPH_ARG(x) (liph_wrapper, (x))
#define LIPH_DECLVAL(type) (liph_wrapper, std::declval<type>())

#define LIPH_COMPILES(type, expr) ((liph::details::make_overloaded( \
                [](long, auto) constexpr { return false; }, \
                [](int, auto *liph_wrapper, std::void_t<decltype(sizeof(*liph_wrapper), (expr), true)>* = 0) constexpr { return true; } \
        )) \
        (0, static_cast<liph::details::wrapper<type>*>(nullptr)))

#define LIPH_HAS_TYPE_MEMBER(type, member) LIPH_COMPILES(type, std::declval<typename std::remove_cv_t<std::remove_reference_t<decltype(LIPH_DECLVAL(type))>>::member>())
#define LIPH_HAS_MEMBER(type, member)      LIPH_COMPILES(type, &std::remove_cv_t<std::remove_reference_t<decltype(LIPH_DECLVAL(type))>>::member)

#define LIPH_CONCEPT(name, expr) \
    template<typename, typename = void> \
    constexpr bool name = false; \
    template<typename T> \
    constexpr bool name<T, std::void_t<expr>> = true


namespace liph {
namespace details {

template<typename... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template<typename... Ts> constexpr auto make_overloaded(Ts... funcs) { return overloaded<Ts...> { funcs... }; }

template<typename... Ts> struct wrapper {};

}
}

#endif
