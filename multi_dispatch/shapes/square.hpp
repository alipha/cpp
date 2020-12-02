#ifndef SQUARE_HPP
#define SQUARE_HPP

#include "shape.hpp"
#include "../multi_dispatch/subclass_registrar.hpp"


class square : public subclass_registrar<shape, square> {
public:
    // (x1, y1) is top-left corner (where (0, 0) is the top-left of the screen)
    square(double x1, double y1, double width) : x1(x1), y1(y1), width(width) {}
    
    bool contains_point(double px, double py) const override { 
        return px >= x1 && px <= x1 + width && py >= y1 && py <= y1 + width;
    }

    double x1, y1, width;
};


#endif
