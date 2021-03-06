#ifndef MULTI_DISPATCH_MULTI_DISPATCH_HPP
#define MULTI_DISPATCH_MULTI_DISPATCH_HPP

#include "subclass_list.hpp"
#include "detail.hpp"
#include <tuple>
#include <type_traits>
#include <utility>


namespace detail {


template<typename RetType, std::size_t TableIndex, typename SubclassTupleTuple, std::size_t... ArgIndexes, typename Func, typename ArgTuple>
RetType dispatcher_impl(std::index_sequence<ArgIndexes...>, Func &&func, ArgTuple &&args) {
    static_assert(std::is_same_v<RetType, decltype(func(cast_arg<TableIndex, ArgIndexes, SubclassTupleTuple>(std::forward<ArgTuple>(args))...))>,
            "All multi-dispatch function overloads must have the same return type");
    return func(cast_arg<TableIndex, ArgIndexes, SubclassTupleTuple>(std::forward<ArgTuple>(args))...);
}


template<std::size_t TableIndex, typename SubclassTupleTuple, typename RetType, typename Func, typename... Superclasses>
RetType dispatcher(Func &&func, Superclasses&&... args) {
    return dispatcher_impl<RetType, TableIndex, SubclassTupleTuple>(std::make_index_sequence<sizeof...(args)>(), std::forward<Func>(func), std::tuple<Superclasses&&...>(std::forward<Superclasses>(args)...));
}


template<typename... SubclassTuples, typename Func, typename... Superclasses, std::size_t... TableIndexes>
auto multi_dispatch_step_2(std::index_sequence<TableIndexes...>, Func &&func, Superclasses&&... args) {
    using RetType = decltype(func(std::get<0>(std::declval<SubclassTuples&&>())...));
    using DispatchType = RetType (*)(Func &&, Superclasses&&...);

    static DispatchType dispatch_table[] = {dispatcher<TableIndexes, std::tuple<SubclassTuples...>, RetType, Func, Superclasses...>...};

    std::size_t index = calc_index<SubclassTuples...>(std::forward<Superclasses>(args)...);
    return dispatch_table[index](std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


template<typename... SubclassTuples, typename Func, typename... Superclasses>
auto multi_dispatch_step_1(Func &&func, Superclasses&&... args) {
    return multi_dispatch_step_2<SubclassTuples...>(std::make_index_sequence<(std::tuple_size_v<SubclassTuples> * ...)>(), std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


} // namespace detail



template<typename... SubclassLists, typename Func, typename... Superclasses>
auto multi_dispatch(Func &&func, Superclasses&&... args) {
    static_assert(sizeof...(SubclassLists) > 0, "At least one subclass_list template parameter to multi_dispatch must be provided");
    static_assert((detail::is_subclass_list<SubclassLists> && ...), "Template parameters to multi_dispatch must be subclass_list types");
    static_assert((detail::validate_superclass_v<SubclassLists> && ...), "A superclass in SubclassLists does not derive from superclass_registrar<Superclass>");
    static_assert((detail::validate_subclasses_v<SubclassLists> && ...), "A subclass in SubclassLists does not derive from subclass_registrar<Superclass, Subclass>");
    static_assert((detail::in_subclass_lists<Superclasses, SubclassLists...> && ...), "Argument to multi_dispatch is not a superclass in one of the specified subclass_lists");

    return detail::multi_dispatch_step_1<detail::get_subclass_tuple_t<Superclasses, SubclassLists...>...>(std::forward<Func>(func), std::forward<Superclasses>(args)...);
}


#endif

