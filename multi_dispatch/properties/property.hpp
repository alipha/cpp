#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#include "../multi_dispatch/superclass_registrar.hpp"
//#include "../multi_dispatch/concrete_superclass_registrar.hpp"
#include <string>
#include <utility>


class property : public /*concrete_*/ superclass_registrar<property> {
public:
    property(std::string name) : name(std::move(name)) {}

    std::string name;
};


#endif
