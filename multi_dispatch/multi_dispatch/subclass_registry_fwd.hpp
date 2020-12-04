#ifndef MULTI_DISPATCH_SUBCLASS_REGISTRY_FWD_HPP
#define MULTI_DISPATCH_SUBCLASS_REGISTRY_FWD_HPP

#include <cstddef>


namespace detail {

template<typename Superclass, typename Subclass>
struct subclass_index_registry {
    static std::size_t value;
};

}  // namespace detail


template<typename SubclassList>
struct subclass_registry {
private:
    static bool populate_subclass_index_registry_values;
};




#endif

