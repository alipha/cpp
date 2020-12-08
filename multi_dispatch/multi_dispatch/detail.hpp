#ifndef MULTI_DISPATCH_DETAIL_HPP
#define MULTI_DISPATCH_DETAIL_HPP

#include <tuple>
#include <type_traits>
#include <utility>


namespace detail {


// TODO: specialize for unique_ptr and shared_ptr

    
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



template<typename...>
std::size_t calc_index() { return 0; }


template<typename FirstSubclassTuple, typename... RestSubclassTuples, typename FirstSuperclass, typename... RestSuperclasses>
std::size_t calc_index(FirstSuperclass &&firstArg, RestSuperclasses&&... restArgs) {
    return firstArg.subclass_index() + std::tuple_size_v<FirstSubclassTuple> * calc_index<RestSubclassTuples...>(std::forward<RestSuperclasses>(restArgs)...);
}



} // namespace detail


#endif
