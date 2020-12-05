#ifndef MULTI_DISPATCH_MULTI_DISPATCH_HPP
#define MULTI_DISPATCH_MULTI_DISPATCH_HPP

#include "subclass_list.hpp"
#include <tuple>
#include <type_traits>
#include <utility>


namespace detail {


template<typename Target, typename Src>
struct match_const_volatile { using type = Src; };

template<typename Target, typename Src>
struct match_const_volatile<const Target, Src> { using type = const Src; };

template<typename Target, typename Src>
struct match_const_volatile<volatile Target, Src> { using type = volatile Src; };

template<typename Target, typename Src>
struct match_const_volatile<const volatile Target, Src> { using type = const volatile Src; };

template<typename Target, typename Src>
using match_cv = typename match_const_volatile<Target, Src>::type;



template<typename Target, typename Src>
struct match_qualifiers { using type = Src; };

template<typename Target, typename Src>
struct match_qualifiers<Target &, Src> { using type = match_cv<Target, Src> &; };

//template<typename Target, typename Src>
//struct match_qualifiers<Target &&, Src> { using type = match_cv<Target, Src> &&; };

template<typename Target, typename Src>
struct match_qualifiers<Target *, Src> { using type = match_cv<Target, Src> *; };

template<typename Target, typename Src>
struct match_qualifiers<Target *&, Src> { using type = match_cv<Target, Src> *&; };

//template<typename Target, typename Src>
//struct match_qualifiers<Target *&&, Src> { using type = match_cv<Target, Src> *&&; };

template<typename Target, typename Src>
struct match_qualifiers<Target *const, Src> { using type = match_cv<Target, Src> *const; };

template<typename Target, typename Src>
struct match_qualifiers<Target *const &, Src> { using type = match_cv<Target, Src> *const &; };

template<typename Target, typename Src>
struct match_qualifiers<Target *volatile, Src> { using type = match_cv<Target, Src> *volatile; };

template<typename Target, typename Src>
struct match_qualifiers<Target *volatile &, Src> { using type = match_cv<Target, Src> *volatile &; };

template<typename Target, typename Src>
struct match_qualifiers<Target *const volatile, Src> { using type = match_cv<Target, Src> *const volatile; };

template<typename Target, typename Src>
struct match_qualifiers<Target *const volatile &, Src> { using type = match_cv<Target, Src> *const volatile &; };



template<typename T>
using remove_qualifiers = std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<T>>>>;



template<typename Target, typename Tuple>
struct tuple_match_qualifiers { 
    static_assert(sizeof(Tuple) && false, "Tuple is not a std::tuple");
};

template<typename Target, typename... Args>
struct tuple_match_qualifiers<Target, std::tuple<Args...>> {
    using type = std::tuple<typename match_qualifiers<Target, Args>::type...>;
};

template<typename Target, typename Tuple>
using tuple_match_qualifiers_t = typename tuple_match_qualifiers<Target, Tuple>::type;



template<typename T>
constexpr bool is_subclass_list = false;

template<typename Superclass, typename SubclassList>
constexpr bool is_subclass_list<subclass_list<Superclass, SubclassList>> = true;


template<typename Superclass, typename... SubclassLists>
constexpr bool in_subclass_lists = (std::is_same_v<remove_qualifiers<Superclass>, typename SubclassLists::superclass> || ...);



template<typename Superclass, typename... SubclassLists>
struct get_subclass_tuple { using type = void; };

template<typename Superclass, typename FirstSubclassList, typename... RestSubclassLists>
struct get_subclass_tuple<Superclass, FirstSubclassList, RestSubclassLists...> {
    using type = 
        std::conditional_t<std::is_same_v<Superclass, typename FirstSubclassList::superclass>,
            typename FirstSubclassList::subclass_tuple,
            typename get_subclass_tuple<Superclass, RestSubclassLists...>::type>;
};

template<typename Superclass, typename... SubclassLists>
using get_subclass_tuple_t = tuple_match_qualifiers_t<Superclass, typename get_subclass_tuple<remove_qualifiers<Superclass>, SubclassLists...>::type>;



template<typename SubclassList>
constexpr bool validate_superclass_v = std::is_base_of_v<detail::superclass_registrar_checker<typename SubclassList::superclass>, typename SubclassList::superclass>;

template<typename SubclassList>
struct validate_subclasses {};

template<typename Superclass, typename... Subclasses>
struct validate_subclasses<subclass_list<Superclass, std::tuple<Subclasses...>>> {
    static constexpr bool value = (std::is_base_of_v<detail::subclass_registrar_checker<Superclass, Subclasses>, Subclasses> && ...);
};

template<typename SubclassList>
constexpr bool validate_subclasses_v = validate_subclasses<SubclassList>::value;



template<typename SubclassTupleTuple, std::size_t... Indexes>
constexpr std::size_t multiply_sizes(std::index_sequence<Indexes...>) {
    return (1 * ... * std::tuple_size_v<std::tuple_element_t<Indexes, SubclassTupleTuple>>);
}


template<std::size_t TableIndex, std::size_t ArgIndex, typename SubclassTupleTuple, typename ArgTuple>
constexpr auto &&cast_arg(ArgTuple &&args) {
    constexpr std::size_t multiplier = multiply_sizes<SubclassTupleTuple>(std::make_index_sequence<ArgIndex>());

    using SubclassTuple = std::tuple_element_t<ArgIndex, SubclassTupleTuple>;
    constexpr std::size_t options = std::tuple_size_v<SubclassTuple>;
    constexpr std::size_t option = (TableIndex / multiplier) % options;
    using Subclass = std::tuple_element_t<option, SubclassTuple>;

    return static_cast<Subclass&&>(std::get<ArgIndex>(std::forward<ArgTuple>(args)));
}


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


template<typename...>
std::size_t calc_index() { return 0; }


template<typename FirstSubclassTuple, typename... RestSubclassTuples, typename FirstSuperclass, typename... RestSuperclasses>
std::size_t calc_index(FirstSuperclass &&firstArg, RestSuperclasses&&... restArgs) {
    return firstArg.subclass_index() + std::tuple_size_v<FirstSubclassTuple> * calc_index<RestSubclassTuples...>(std::forward<RestSuperclasses>(restArgs)...);
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

