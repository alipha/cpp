#ifndef MULTI_DISPATCH_SUBCLASS_LIST_HPP
#define MULTI_DISPATCH_SUBCLASS_LIST_HPP

#include <tuple>
#include <type_traits>


template<typename Superclass, typename SubclassTuple>
struct subclass_list {
    static_assert(sizeof(Superclass) && false, "subclass_list's second template argument must be a std::tuple of subclasses");
};

template<typename Superclass, typename... Subclasses>
struct subclass_list<Superclass, std::tuple<Subclasses...>> {
    static_assert(sizeof...(Subclasses), "At least one subclass must be listed in the std::tuple");
    static_assert(std::is_class_v<Superclass>, "Superclass must be a class/struct type");
    static_assert((std::is_class_v<Subclasses> && ...), "Subclasses must be class/struct types");

    using superclass = Superclass;
    using subclass_tuple = std::tuple<Subclasses...>;
};

#endif

