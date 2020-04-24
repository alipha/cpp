#ifndef LIPH_CURRY_HPP
#define LIPH_CURRY_HPP

#include <type_traits>
#include <tuple>
#include <utility>


namespace liph {

namespace detail {


template <typename T1, typename... T, size_t... Indices>
std::tuple<T...> unshift_tuple_with_indices(const std::tuple<T1, T...>& tuple, std::index_sequence<Indices...>)
{
    return std::make_tuple(std::get<Indices + 1>(tuple)...);
}


} // namespace detail



template <typename T1, typename... T> 
std::tuple<T...> unshift_tuple(const std::tuple<T1, T...>& tuple)
{
    return detail::unshift_tuple_with_indices(tuple, std::make_index_sequence<sizeof...(T)>());
}



template<typename Func, typename... State>
struct curry {

    curry(Func f, State... s) : state(std::move(f), std::move(s)...) {}
    
    template<typename... T>
    auto operator()(T&&... args) { return apply(0, std::forward<T>(args)...); }
    
private:
    template<typename... T, typename = std::void_t<decltype(std::declval<Func>()(std::declval<State>()..., std::declval<T>()...))>*>
    auto apply(int, T&&... args) {
        return std::apply(std::get<0>(state), std::tuple_cat(unshift_tuple(state), std::make_tuple(std::forward<T>(args)...)));
    }
    
    template<typename... T>
    auto apply(long, T&&... args) {
        return std::make_from_tuple<curry<Func, State..., T...>>(std::tuple_cat(state, std::make_tuple(std::forward<T>(args)...)));
    }
    
    std::tuple<Func, State...> state;
};


} // namespace liph

#endif
