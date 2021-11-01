/*
MIT License

Copyright (c) 2021 Kevin Spinar (Alipha)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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
