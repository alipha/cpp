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



template<typename SubclassTupleTuple, std::size_t... Indexes>
constexpr std::size_t multiply_sizes(std::index_sequence<Indexes...>) {
    return (1 * ... * std::tuple_size_v<std::remove_reference_t<decltype(std::get<Indexes>(std::declval<SubclassTupleTuple&>()))>>);
}


template<std::size_t TableIndex, std::size_t ArgIndex, typename SubclassTupleTuple, typename ArgTuple>
constexpr auto cast_arg(ArgTuple &&args) {
    constexpr std::size_t multiplier = multiply_sizes<SubclassTupleTuple>(std::make_index_sequence<ArgIndex>());

    using SubclassTuple = std::remove_reference_t<decltype(std::get<ArgIndex>(std::declval<SubclassTupleTuple&>()))>;
    constexpr std::size_t options = std::tuple_size_v<SubclassTuple>;
    constexpr std::size_t option = (TableIndex / multiplier) % options;
    using Subclass = std::remove_reference_t<decltype(std::get<option>(std::declval<SubclassTuple&>()))>;

    return static_cast<Subclass&>(std::get<ArgIndex>(std::forward<ArgTuple>(args)));
}


template<typename RetType, std::size_t TableIndex, typename SubclassTupleTuple, std::size_t... ArgIndexes, typename Func, typename ArgTuple>
RetType dispatcher_impl(std::index_sequence<ArgIndexes...>, Func &&func, ArgTuple &&args) {
    static_assert(std::is_same_v<RetType, decltype(func(get_arg<TableIndex, ArgIndexes, SubclassTupleTuple>(std::forward<ArgTuple>(args))...)),
            "All multi-dispatch function overloads must have the same return type");
    return func(cast_arg<TableIndex, ArgIndexes, SubclassTupleTuple>(std::forward<ArgTuple>(args))...);
}


template<typename RetType, typename Func, typename... Superclasses, typename SubclassTupleTuple, std::size_t TableIndex>
RetType dispatcher(Func &&func, Superclasses&&... args) {
    return dispatcher_impl<RetType, TableIndex, SubclassTupleTuple>(std::make_index_sequence<sizeof...(args)>(), std::forward<Func>(func), std::tie(std::forward<Superclasses>(args)...));
}


std::size_t calc_index() { return 0; }


template<typename FirstSubclassTuple, typename... RestSubclassTuples, typename FirstSuperclass, typename... RestSuperclasses>
std::size_t calc_index(FirstSuperclass &&firstArg, RestSuperclasses&&... restArgs) {
    return firstArg.subclass_index() + std::tuple_size_v<FirstSubclassTuple> * calc_index<RestSubclassTuples...>(std::forward<RestSuperclasses...>(restArgs));
}


template<typename... SubclassTuples, typename Func, typename... Superclasses, std::size_t... TableIndexes>
auto multi_dispatch_step_2(Func &&func, Superclasses&&... args, std::index_sequence<Indexes...>) {
    using RetType = decltype(func(std::get<0>(std::declval<SubclassTuples&>())...));
    using DispatchType = RetType (*)(Func &&, Superclasses&&...);

    static DispatchType dispatch_table[] = {dispatcher<RetType, Func, Superclasses..., std::tuple<SubclassTuples...>, TableIndexes>...};

    std::size_t index = calc_index<SubclassTuples...>(std::forward<Superclasses>(args)...);
    return dispatch_table[index](std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


template<typename... SubclassTuples, typename Func, typename... Superclasses>
auto multi_dispatch_step_1(Func &&func, Superclasses&&... args) {
    return multi_dispatch_step_2<SubclassTuples...>(std::forward<Func>(func), std::forward<Superclasses>(args)..., std::make_index_sequence<std::tuple_size_v<SubclassTuples> * ...>());
}


} // namespace detail



template<typename... SubclassLists, typename Func, typename... Superclasses>
auto multi_dispatch(Func &&func, Superclasses&&... args) {
    static_assert(sizeof...(SubclassLists) > 0, "At least one subclass_list template parameter to multi_dispatch must be provided");
    static_assert((detail::is_subclass_list<SubclassLists> && ...), "Template parameters to multi_dispatch must be subclass_list types");
    static_assert((detail::in_subclass_lists<Superclasses, SubclassLists...> && ...), "Argument to multi_dispatch is not a superclass in one of the specified subclass_lists");

    return detail::multi_dispatch_step_1<detail::get_subclass_tuple_t<Superclasses, SubclassLists...>...>(std::forward<Func>(func), std::forward<Superclasses>(args)...);
}



/*
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
*/



}

#endif

