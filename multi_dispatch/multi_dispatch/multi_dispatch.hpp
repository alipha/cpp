#ifndef MULTI_DISPATCH_MULTI_DISPATCH_HPP
#define MULTI_DISPATCH_MULTI_DISPATCH_HPP

#include "subclass_list.hpp"
#include <tuple>
#include <type_traits>
#include <utility>


namespace detail {

    
template<typename T>
constexpr bool is_subclass_list = false;

template<typename Superclass, typename SubclassList>
constexpr bool is_subclass_list<subclass_list<Superclass, SubclassList>> = true;


template<typename Superclass, typename... SubclassLists>
constexpr bool in_subclass_lists = (std::is_same_v<Superclass, typename SubclassLists::superclass> || ...);



template<typename Superclass, typename... SubclassLists>
struct get_subclass_tuple { using type = void; };

template<typename Superclass, typename FirstSubclassList, typename... RestSubclassLists>
struct get_subclass_tuple<Superclass, FirstSubclassList, RestSubclassLists...> {
    using type = std::conditional_t<std::is_same_v<Superclass, typename FirstSubclassList::superclass>,
        typename FirstSubclassList::subclass_tuple,
        typename get_subclass_tuple<Superclass, RestSubclassLists...>::type>;
};

template<typename Superclass, typename FirstSubclassList, typename... RestSubclassLists>
using get_subclass_tuple_t = typename get_subclass_tuple<Superclass, FirstSubclassList, RestSubclassLists...>::type;


} // namespace detail


template<typename RetType, typename Func, typename Superclass, typename SubclassTuple, std::size_t Index>
RetType dispatcher(Func &&func, Superclass &&arg) {
    using Subclass = decltype(std::get<Index>(std::declval<SubclassTuple&>()));
    return func(static_cast<Subclass&>(arg));
}


template<typename SubclassTuple, typename Func, typename Superclass, std::size_t... Indexes>
auto do_multi_dispatch(Func &&func, Superclass &&arg, std::index_sequence<Indexes...>) {
    using FirstSubclass = decltype(std::get<0>(std::declval<SubclassTuple&>()));
    using RetType = decltype(func(std::declval<FirstSubclass&>()));
    using DispatchType = RetType (*)(Func &&, Superclass &&);

    static DispatchType dispatch_table[sizeof...(Indexes)] = {dispatcher<RetType, Func, Superclass, SubclassTuple, Indexes>...};

    return dispatch_table[arg.subclass_index()](std::forward<Func>(func), std::forward<Superclass>(arg));
}


template<typename... SubclassLists, typename Func, typename Superclass>
auto multi_dispatch(Func &&func, Superclass &&arg) {
    static_assert(sizeof...(SubclassLists) > 0, "At least one subclass_list template parameter to multi_dispatch must be provided");
    static_assert((detail::is_subclass_list<SubclassLists> && ...), "Template parameters to multi_dispatch must be subclass_list types");
    static_assert(detail::in_subclass_lists<Superclass, SubclassLists...>, "Argument to multi_dispatch is not a superclass in one of the specified subclass_lists");

    using SubclassTuple = get_subclass_tuple_t<Superclass, SubclassLists...>;

    return do_multi_dispatch<SubclassTuple>(std::forward<Func>(func), std::forward<Superclass>(arg), std::make_index_sequence<std::tuple_size_v<SubclassTuple>>);
}

template<typename... SubclassLists, typename Func, typename FirstSuperclass, typename SecondSuperclass, typename... RestSuperclasses>
auto multi_dispatch(Func &&func, FirstSuperclass &&firstArg, SecondSuperclass &&secondArg, RestSuperclasses&&.. restArgs) {


}

#endif

