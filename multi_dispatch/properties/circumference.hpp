#ifndef CIRCUMFERENCE_HPP
#define CIRCUMFERENCE_HPP

#include "property.hpp"
#include "../multi_dispatch/subclass_registrar.hpp"


class circumference : public subclass_registrar<property, circumference> {
public:
    circumference() : subclass_registrar<property, circumference>("circumference") {} 
};


#endif

