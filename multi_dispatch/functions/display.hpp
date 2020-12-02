#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <iostream>

class shape;

std::ostream &operator<<(std::ostream &os, const shape &s);

#endif
