#include "contains.hpp"
#include "../shapes/all_shapes.hpp"
#include "../multi_dispatch/multi_dispatch.hpp"


namespace impl {
   
    bool contains_shape(const circle &outside, const circle &inside) {
    }
   
    bool contains_shape(const circle &outside, const rectangle &inside) {
    }
   
    bool contains_shape(const circle &outside, const square &inside) {
    }
   
    bool contains_shape(const rectangle &outside, const circle &inside) {
    }
   
    bool contains_shape(const rectangle &outside, const rectangle &inside) {
    }
   
    bool contains_shape(const rectangle &outside, const square &inside) {
    }
   
    bool contains_shape(const square &outside, const circle &inside) {
    }
   
    bool contains_shape(const square &outside, const rectangle &inside) {
    }
   
    bool contains_shape(const square &outside, const square &inside) {
    }
}


bool contains_shape(const shape &outside, const shape &inside) {
    return multi_dispatch<shapes>([](auto &outside, auto &inside) {
        return impl::contains_shape(outside, inside);
    }, outside, inside);
}

