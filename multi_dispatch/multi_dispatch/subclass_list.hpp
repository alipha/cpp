#ifndef MULTI_DISPATCH_SUBCLASS_LIST_HPP
#define MULTI_DISPATCH_SUBCLASS_LIST_HPP

#include <tuple>


template<typename Superclass, typename SubclassTuple>
struct subclass_list {
    static_assert(sizeof(Superclass) && false, "subclass_list's second template argument must be a std::tuple of subclasses");
};

template<typename Superclass, typename... Subclasses>
struct subclass_list<Superclass, std::tuple<Subclasses...>> {
    static_assert(sizeof...(Subclasses), "At least one subclass must be listed in the std::tuple");

    using superclass = Superclass;
    using subclass_tuple = std::tuple<Subclasses...>;
};

#endif

