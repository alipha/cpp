#ifndef MULTI_DISPATCH_SUBCLASS_REGISTRAR_HPP
#define MULTI_DISPATCH_SUBCLASS_REGISTRAR_HPP

#include "subclass_registry_fwd.hpp"
#include "superclass_registrar.hpp"
#include <cstddef>
#include <type_traits>


namespace detail {

template<typename Superclass, typename Subclass>
struct subclass_registrar_checker {};

}


template<typename Superclass, typename Subclass, typename ImmediateSuperclass = Superclass>
struct subclass_registrar : detail::subclass_registrar_checker<Superclass, Subclass>, ImmediateSuperclass {
    static_assert(std::is_base_of_v<Superclass, ImmediateSuperclass>, "ImmediateSuperclass does not derive from Superclass");
    static_assert(std::is_base_of_v<detail::superclass_registrar_checker<Superclass>, Superclass>, "Superclass does not derive from superclass_registrar<Superclass>");

    using ImmediateSuperclass::ImmediateSuperclass;

    std::size_t subclass_index() const override {
        return detail::subclass_index_registry<Superclass, Subclass>::value;
    }
};

#endif


