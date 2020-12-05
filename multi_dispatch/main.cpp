#include "shapes/all_shapes.hpp"
#include "properties/all_properties.hpp"
#include "functions/compute_property.hpp"
#include <iostream>


void show(const shape &s) {
    std::cout << s << '\n';
    std::cout << "         area: " << compute_property(s, area()) << '\n';
    std::cout << "circumference: " << compute_property(s, circumference()) << "\n\n";
}


int main() {
    square s(2, 2, 1);
    rectangle r(2.5, 2.5, 8, 3);
    circle c(0, 0, 3);
    
    //std::cout << collides(s, r) << collides(r, c) << collides(s, c) << std::endl;
    show(s);
    show(r);
    show(c);
}
