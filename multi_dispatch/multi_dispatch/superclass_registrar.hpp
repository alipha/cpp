#ifndef MULTI_DISPATCH_SUPERCLASS_REGISTRAR_HPP
#define MULTI_DISPATCH_SUPERCLASS_REGISTRAR_HPP

#include <cstddef>


namespace detail {

template<typename T>
struct superclass_registrar_checker {};

}


template<typename T, typename Base = void>
struct superclass_registrar : detail::superclass_registrar_checker<T>, Base {
    using Base::Base;
    virtual std::size_t subclass_index() const = 0;
};


template<typename T>
struct superclass_registrar<T, void> : detail::superclass_registrar_checker<T> {
    virtual std::size_t subclass_index() const = 0; 
};

#endif

