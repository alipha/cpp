#ifndef SHAPES_FWD_HPP
#define SHAPES_FWD_HPP

#include "../multi_dispatch/subclass_list.hpp"

using shapes = subclass_list<class shape, std::tuple<class circle, class square, class rectangle>>;

#endif
