#include "contains.hpp"
#include "../shapes/all_shapes.hpp"
#include "../multi_dispatch/multi_dispatch.hpp"


namespace impl {
   
    bool contains_shape(const circle &outside, const circle &inside) {
        return distance(outside.x, outside.y, inside.x, inside.y) + inside.radius <= outside.radius;
    }
   
    bool contains_shape(const shape &outside, const rectangle &inside) {
        return outside.contains_point(inside.x1, inside.y1)
            && outside.contains_point(inside.x1, inside.y2)
            && outside.contains_point(inside.x2, inside.y1)
            && outside.contains_point(inside.x2, inside.y2);

    }
   
    bool contains_shape(const shape &outside, const square &inside) {
        return outside.contains_point(inside.x1, inside.y1)
            && outside.contains_point(inside.x1, inside.y1 + inside.width)
            && outside.contains_point(inside.x1 + inside.width, inside.y1)
            && outside.contains_point(inside.x1 + inside.width, inside.y1 + inside.width);
    }
   
    bool contains_shape(const shape &outside, const circle &inside) {
        return outside.contains_point(inside.x - inside.radius, inside.y)
            && outside.contains_point(inside.x + inside.radius, inside.y)
            && outside.contains_point(inside.x, inside.y - inside.radius)
            && outside.contains_point(inside.x, inside.y + inside.radius);
    }  
}


bool contains_shape(const shape &outside, const shape &inside) {
    return multi_dispatch<shapes>([](auto &outside, auto &inside) {
        return impl::contains_shape(outside, inside);
    }, outside, inside);
}

