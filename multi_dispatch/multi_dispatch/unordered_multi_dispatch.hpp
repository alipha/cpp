#ifndef MULTI_DISPATCH_UNORDERED_MULTI_DISPATCH_HPP
#define MULTI_DISPATCH_UNORDERED_MULTI_DISPATCH_HPP

#include "subclass_list.hpp"
#include "detail.hpp"
#include <tuple>
#include <type_traits>
#include <utility>


namespace detail {


template<std::size_t Needle, std::size_t... BeforeIndexes, std::size_t CurrentIndex, std::size_t... AfterIndexes>
auto remove_index_impl(std::index_sequence<BeforeIndexes...>, std::index_sequence<CurrentIndex, AfterIndexes...>) {
    if constexpr(Needle == CurrentIndex) {
        return std::index_sequence<BeforeIndexes..., AfterIndexes...>();
    } else {
        return remove_index_impl<Needle>(std::index_sequence<BeforeIndexes..., CurrentIndex>(), std::index_sequence<AfterIndexes...>());
    }
}

template<std::size_t Needle, std::size_t... Haystack>
auto remove_index(std::index_sequence<Haystack...> seq) {
    return remove_index_impl<Needle>(std::make_index_sequence<0>(), seq);
}

template<std::size_t... PrefixIndexes, std::size_t... AvailIndexes>
auto get_permutations_impl(std::index_sequence<PrefixIndexes...>, std::index_sequence<AvailIndexes...> avail) {
    (void)avail;
    if constexpr(sizeof...(AvailIndexes) == 0) {
        return std::tuple<std::index_sequence<PrefixIndexes...>>();
    } else {
        return std::tuple_cat(get_permutations_impl(std::index_sequence<PrefixIndexes..., AvailIndexes>(), remove_index<AvailIndexes>(avail))...);
    }
}

template<std::size_t N>
auto get_permutations() { return get_permutations_impl(std::make_index_sequence<0>(), std::make_index_sequence<N>()); }


template<typename... Args>
auto make_ref_tuple(Args&&... args) {
    return std::tuple<Args&&...>(std::forward<Args>(args)...);
}


template<typename ArgTuple, std::size_t... ReorderedIndexes>
auto reorder_args(std::index_sequence<ReorderedIndexes...>, ArgTuple &&args) {
    return make_ref_tuple(std::get<ReorderedIndexes>(std::forward<ArgTuple>(args))...);
}


template<template<typename...> typename Validator, typename RetType, std::size_t TableIndex, typename SubclassTupleTuple, typename PermTuple, typename Func, typename ArgTuple, std::size_t FirstPermIndex, std::size_t... RestPermIndexes, std::size_t... ArgIndexes>
RetType un_dispatcher_impl(std::index_sequence<FirstPermIndex, RestPermIndexes...>, std::index_sequence<ArgIndexes...> argSeq, Func &&func, ArgTuple &&args) {
    
    auto castedArgs = make_ref_tuple(cast_arg<TableIndex, ArgIndexes, SubclassTupleTuple>(std::forward<ArgTuple>(args))...);
    auto reorderedArgs = reorder_args(std::tuple_element_t<FirstPermIndex, PermTuple>(), std::forward<decltype(castedArgs)>(castedArgs));

    if constexpr(Validator<std::tuple_element_t<ArgIndexes, decltype(reorderedArgs)>...>::value) {
        (void)argSeq;
        static_assert(std::is_same_v<RetType, decltype(func(std::get<ArgIndexes>(std::forward<decltype(reorderedArgs)>(reorderedArgs))...))>,
            "All multi-dispatch function overloads must have the same return type");
        return func(std::get<ArgIndexes>(std::forward<decltype(reorderedArgs)>(reorderedArgs))...);
    } else {
        static_assert(sizeof...(RestPermIndexes) > 0, "The arguments to unordered_multi_dispatch cannot be applied no matter the order");
        return un_dispatcher_impl<Validator, RetType, TableIndex, SubclassTupleTuple, PermTuple>(
            std::index_sequence<RestPermIndexes...>(), argSeq, std::forward<Func>(func), std::forward<ArgTuple>(args));
    }
}


template<template<typename...> typename Validator, std::size_t TableIndex, typename SubclassTupleTuple, typename RetType, typename Func, typename... Superclasses>
RetType un_dispatcher(Func &&func, Superclasses&&... args) {
    using PermTuple = decltype(get_permutations<sizeof...(args)>()); 
    return un_dispatcher_impl<Validator, RetType, TableIndex, SubclassTupleTuple, PermTuple>(
            std::make_index_sequence<std::tuple_size_v<PermTuple>>(), 
            std::make_index_sequence<sizeof...(Superclasses)>(), 
            std::forward<Func>(func), 
            std::tuple<Superclasses&&...>(std::forward<Superclasses>(args)...));
}


template<template<typename...> typename Validator, typename... SubclassTuples, typename Func, typename... Superclasses, std::size_t... TableIndexes>
auto un_multi_dispatch_step_2(std::index_sequence<TableIndexes...>, Func &&func, Superclasses&&... args) {
    using RetType = decltype(func(std::get<0>(std::declval<SubclassTuples&&>())...));
    using DispatchType = RetType (*)(Func &&, Superclasses&&...);

    static DispatchType dispatch_table[] = {un_dispatcher<Validator, TableIndexes, std::tuple<SubclassTuples...>, RetType, Func, Superclasses...>...};

    std::size_t index = calc_index<SubclassTuples...>(std::forward<Superclasses>(args)...);
    return dispatch_table[index](std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


template<template<typename...> typename Validator, typename... SubclassTuples, typename Func, typename... Superclasses>
auto un_multi_dispatch_step_1(Func &&func, Superclasses&&... args) {
    return un_multi_dispatch_step_2<Validator, SubclassTuples...>(std::make_index_sequence<(std::tuple_size_v<SubclassTuples> * ...)>(), std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


} // namespace detail



template<template<typename...> typename Validator, typename... SubclassLists, typename Func, typename... Superclasses>
auto unordered_multi_dispatch(Func &&func, Superclasses&&... args) {
    static_assert(sizeof...(args) <= 4, "Too many arguments. The compiler cannot handle all the template generation from all the permutations of parameters");
    static_assert(sizeof...(SubclassLists) > 0, "At least one subclass_list template parameter to multi_dispatch must be provided");
    static_assert((detail::is_subclass_list<SubclassLists> && ...), "Template parameters to multi_dispatch must be subclass_list types");
    static_assert((detail::validate_superclass_v<SubclassLists> && ...), "A superclass in SubclassLists does not derive from superclass_registrar<Superclass>");
    static_assert((detail::validate_subclasses_v<SubclassLists> && ...), "A subclass in SubclassLists does not derive from subclass_registrar<Superclass, Subclass>");
    static_assert((detail::in_subclass_lists<Superclasses, SubclassLists...> && ...), "Argument to multi_dispatch is not a superclass in one of the specified subclass_lists");

    return detail::un_multi_dispatch_step_1<Validator, detail::get_subclass_tuple_t<Superclasses, SubclassLists...>...>(std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


#endif

