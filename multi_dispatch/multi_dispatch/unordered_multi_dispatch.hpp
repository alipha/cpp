#ifndef MULTI_DISPATCH_UNORDERED_MULTI_DISPATCH_HPP
#define MULTI_DISPATCH_UNORDERED_MULTI_DISPATCH_HPP

#include "subclass_list.hpp"
#include "detail.hpp"
#include <tuple>
#include <type_traits>
#include <utility>


namespace detail {


constexpr std::size_t factorial(std::size_t n) {
    std::size_t result = 1;
    for(std::size_t i = 2; i <= n; ++i)
        result *= i;
    return result;
}


template<std::size_t N>
constexpr std::size_t nth_avail(const std::array<bool, N> &taken, std::size_t n) {
    for(std::size_t i = 0; i < N; ++i) {
        if(!taken[i]) {
            if(n-- == 0)
                return i;
        }
    }
    return N; 
}


template<std::size_t N, std::size_t PermIndex>
constexpr std::array<std::size_t, N> permutation() {
    std::array<std::size_t, N> result = {};
    std::array<bool, N> taken = {};
    
    std::size_t perm_index = PermIndex;
    
    for(std::size_t i = 0; i < N; ++i) {
        std::size_t index = nth_avail(taken, perm_index % (N - i));
        result[i] = index;
        taken[index] = true;
        perm_index /= N - i;
    }
    
    return result;
}


template<std::size_t N, std::size_t... Indexes>
constexpr std::array<std::array<std::size_t, N>, sizeof...(Indexes)> permutations_impl(std::index_sequence<Indexes...>) {
    return {permutation<N, Indexes>()...};
}


template<std::size_t N>
constexpr auto permutations() {
    return permutations_impl<N>(std::make_index_sequence<factorial(N)>());
}


template<typename... Args>
auto make_ref_tuple(Args&&... args) {
    return std::tuple<Args&&...>(std::forward<Args>(args)...);
}


template<std::size_t PermIndex, typename ArgTuple, std::size_t... Indexes>
auto reorder_args(std::index_sequence<Indexes...>, ArgTuple &&args) {
    constexpr std::array<std::size_t, sizeof...(Indexes)> perm = permutation<sizeof...(Indexes), PermIndex>();
    return make_ref_tuple(std::get<perm[Indexes]>(std::forward<ArgTuple>(args))...);
}


template<template<typename...> typename Validator, typename RetType, std::size_t TableIndex, typename SubclassTupleTuple, std::size_t PermIndex, std::size_t PermSize, typename Func, typename ArgTuple, std::size_t... ArgIndexes>
RetType un_dispatcher_impl(std::index_sequence<ArgIndexes...> argSeq, Func &&func, ArgTuple &&args) {
    
    auto castedArgs = make_ref_tuple(cast_arg<TableIndex, ArgIndexes, SubclassTupleTuple>(std::forward<ArgTuple>(args))...);
    auto reorderedArgs = reorder_args<PermIndex>(argSeq, std::forward<decltype(castedArgs)>(castedArgs));

    if constexpr(Validator<std::tuple_element_t<ArgIndexes, decltype(reorderedArgs)>...>::value) {
        (void)argSeq;
        static_assert(std::is_same_v<RetType, decltype(func(std::get<ArgIndexes>(std::forward<decltype(reorderedArgs)>(reorderedArgs))...))>,
            "All multi-dispatch function overloads must have the same return type");
        return func(std::get<ArgIndexes>(std::forward<decltype(reorderedArgs)>(reorderedArgs))...);
    } else {
        static_assert(PermIndex < PermSize, "The arguments to unordered_multi_dispatch cannot be applied no matter the order");
        return un_dispatcher_impl<Validator, RetType, TableIndex, SubclassTupleTuple, PermIndex + 1, PermSize>(
            argSeq, std::forward<Func>(func), std::forward<ArgTuple>(args));
    }
}


template<template<typename...> typename Validator, std::size_t TableIndex, typename SubclassTupleTuple, typename RetType, typename Func, typename... Superclasses>
RetType un_dispatcher(Func &&func, Superclasses&&... args) {
    return un_dispatcher_impl<Validator, RetType, TableIndex, SubclassTupleTuple, 0, factorial(sizeof...(args))>( 
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
    static_assert(sizeof...(SubclassLists) > 0, "At least one subclass_list template parameter to multi_dispatch must be provided");
    static_assert((detail::is_subclass_list<SubclassLists> && ...), "Template parameters to multi_dispatch must be subclass_list types");
    static_assert((detail::validate_superclass_v<SubclassLists> && ...), "A superclass in SubclassLists does not derive from superclass_registrar<Superclass>");
    static_assert((detail::validate_subclasses_v<SubclassLists> && ...), "A subclass in SubclassLists does not derive from subclass_registrar<Superclass, Subclass>");
    static_assert((detail::in_subclass_lists<Superclasses, SubclassLists...> && ...), "Argument to multi_dispatch is not a superclass in one of the specified subclass_lists");

    return detail::un_multi_dispatch_step_1<Validator, detail::get_subclass_tuple_t<Superclasses, SubclassLists...>...>(std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


#endif

