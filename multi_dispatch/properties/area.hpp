#ifndef AREA_HPP
#define AREA_HPP

#include "property.hpp"
#include "../multi_dispatch/subclass_registrar.hpp"


class area : public subclass_registrar<property, area> {
public:
    area() : subclass_registrar<property, area>("area") {} 
};


#endif

