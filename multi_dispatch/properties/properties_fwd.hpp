#ifndef PROPERTIES_FWD_HPP
#define PROPERTIES_FWD_HPP

#include "../multi_dispatch/subclass_list.hpp"


class property;
class area;
class circumference;

using properties = subclass_list<property, std::tuple<area, circumference>>;

#endif
