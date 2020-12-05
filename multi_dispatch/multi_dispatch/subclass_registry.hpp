#ifndef MULTI_DISPATCH_SUBCLASS_REGISTRY_HPP
#define MULTI_DISPATCH_SUBCLASS_REGISTRY_HPP

#include "subclass_registry_fwd.hpp"
#include <cstddef>
#include <tuple>


namespace detail {

template<typename Superclass, typename Subclass>
std::size_t subclass_index_registry<Superclass, Subclass>::value = 0;


template<typename SubclassList, std::size_t Index>
bool populate_indexes() {
    using Subclasses = typename SubclassList::subclass_tuple;
    if constexpr(Index < std::tuple_size_v<Subclasses>) {
        using Subclass = std::tuple_element_t<Index, Subclasses>;
        subclass_index_registry<typename SubclassList::superclass, Subclass>::value = Index;
        return populate_indexes<SubclassList, Index + 1>();
    } else {
        return true;
    }
}

} // namespace detail


template<typename SubclassList>
bool subclass_registry<SubclassList>::populate_subclass_index_registry_values 
    = detail::populate_indexes<SubclassList, 0>();




#endif
