#include "collides.hpp"
#include "../shapes/all_shapes.hpp"
#include "../multi_dispatch/unordered_multi_dispatch.hpp"

#include <type_traits>


namespace impl {

    inline bool collides(const circle &left, const circle &right) {
        return distance(left.x, left.y, right.x, right.y) <= left.radius + right.radius;
    }

    inline bool collides(const circle &left, const rectangle &right) {
        return right.contains_point(left.x, left.y - left.radius) 
            || right.contains_point(left.x, left.y + left.radius)
            || right.contains_point(left.x - left.radius, left.y)
            || right.contains_point(left.x + left.radius, left.y)
            || left.contains_point(right.x1, right.y1)
            || left.contains_point(right.x1, right.y2)
            || left.contains_point(right.x2, right.y1)
            || left.contains_point(right.x2, right.y2);
    }

    inline bool collides(const circle &left, const square &right) {
        rectangle r(right.x1, right.y1, right.x1 + right.width, right.y1 + right.width);
        return collides(left, r);
    }

    inline bool collides(const square &left, const square &right) {
        return left.contains_point(right.x1, right.y1)
            || left.contains_point(right.x1, right.y1 + right.width)
            || left.contains_point(right.x1 + right.width, right.y1)
            || left.contains_point(right.x1 + right.width, right.y1 + right.width);
    }

    inline bool collides(const square &left, const rectangle &right) {
        return left.contains_point(right.x1, right.y1)
            || left.contains_point(right.x1, right.y2)
            || left.contains_point(right.x2, right.y1)
            || left.contains_point(right.x2, right.y2);
    }

    inline bool collides(const rectangle &left, const rectangle &right) {
        return left.contains_point(right.x1, right.y1)
            || left.contains_point(right.x1, right.y2)
            || left.contains_point(right.x2, right.y1)
            || left.contains_point(right.x2, right.y2);
    }

}  // namespace impl


template<typename T, typename U, typename = void>
constexpr bool collides_invokeable_v = false;

template<typename T, typename U>
constexpr bool collides_invokeable_v<T, U, std::void_t<decltype(impl::collides(std::declval<T>(), std::declval<U>()))>> = true;

template<typename T, typename U>
struct collides_invokeable { static constexpr bool value = collides_invokeable_v<T, U>; };



bool collides(const shape &left, const shape &right) {
    return unordered_multi_dispatch<collides_invokeable, shapes>([](auto &left, auto &right) {
        return impl::collides(left, right);
    }, left, right);
}

