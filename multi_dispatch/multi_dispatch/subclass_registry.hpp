#ifndef MULTI_DISPATCH_SUBCLASS_REGISTRY_HPP
#define MULTI_DISPATCH_SUBCLASS_REGISTRY_HPP

#include "subclass_registry_fwd.hpp"
#include <cstddef>
#include <tuple>


namespace detail {

template<typename Superclass, typename Subclass>
std::size_t subclass_index_registry<Superclass, Subclass>::value = 0;


template<typename>
bool populate_indexes(std::tuple<>*, std::size_t = 0) { return true; }

template<typename Superclass, typename FirstSubclass, typename... RestSubclasses>
bool populate_indexes(std::tuple<FirstSubclass, RestSubclasses...>*, std::size_t index = 0) {
    subclass_index_registry<Superclass, FirstSubclass>::value = index;
    return populate_indexes((std::tuple<RestSubclasses...>*)nullptr, index + 1);
}

} // namespace detail


template<typename SubclassList>
bool subclass_registry<SubclassList>::populate_subclass_index_registry_values 
    = detail::populate_indexes<Superclass>((typename SubclassList::subclass_tuple*)nullptr);




#endif
