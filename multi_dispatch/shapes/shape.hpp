#ifndef SHAPE_HPP
#define SHAPE_HPP

#include "../functions/collides.hpp"
#include "../functions/contains.hpp"
#include "../functions/display.hpp"
#include "../multi_dispatch/superclass_registrar.hpp"
#include <cmath>


inline double distance(double x1, double y1, double x2, double y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}


class shape : public superclass_registrar<shape> {
public:
    virtual ~shape() {}

    bool contains(const shape &inside) const { return contains_shape(*this, inside); }

    virtual bool contains_point(double x, double y) const = 0;
};


#endif
