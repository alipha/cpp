#include "shapes/all_shapes.hpp"
#include <iostream>


int main() {
    square s(2, 2, 1);
    rectangle r(2.5, 2.5, 8, 3);
    circle c(0, 0, 3);
    
    std::cout << collides(s, r) << collides(r, c) << collides(s, c) << std::endl;
}
