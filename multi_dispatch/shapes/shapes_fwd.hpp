#ifndef SHAPES_FWD_HPP
#define SHAPES_FWD_HPP

#include "../multi_dispatch/subclass_list.hpp"


class shape;
class circle;
class square;
class rectangle;

using shapes = subclass_list<shape, std::tuple<circle, square, rectangle>>;

#endif
