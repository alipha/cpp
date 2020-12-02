#ifndef RECTANGLE_HPP
#define RECTANGLE_HPP

#include "shape.hpp"
#include "../multi_dispatch/subclass_registrar.hpp"


class rectangle : public subclass_registrar<shape, rectangle> {
public:
    rectangle(double x1, double y1, double x2, double y2) : x1(x1), y1(y1), x2(x2), y2(y2) {}
    
    bool contains_point(double px, double py) const override { 
        return px >= x1 && px <= x2 && py >= y1 && py <= y2;
    }

    double x1, y1, x2, y2;
};


#endif
