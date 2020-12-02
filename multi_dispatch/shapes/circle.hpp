#ifndef CIRCLE_HPP
#define CIRCLE_HPP

#include "shape.hpp"
#include "../multi_dispatch/subclass_registrar.hpp"


class circle : public subclass_registrar<shape, circle> {
public:
    circle(double x, double y, double radius) : x(x), y(y), radius(radius) {}
    
    bool contains_point(double px, double py) const override {
        return distance(x, y, px, py) <= radius;
    }

    double x, y, radius;
};


#endif
