#ifndef MULTI_DISPATCH_SUPERCLASS_REGISTRAR_HPP
#define MULTI_DISPATCH_SUPERCLASS_REGISTRAR_HPP

#include <cstddef>


template<typename T, typename Base = void>
struct superclass_registrar : Base {
    using Base::Base;
    virtual std::size_t subclass_index() const = 0;
};


template<typename T>
struct superclass_registrar<T, void> {
    virtual std::size_t subclass_index() const = 0; 
};

#endif

