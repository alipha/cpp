#include "display.hpp"
#include "../shapes/all_shapes.hpp"
#include "../multi_dispatch/multi_dispatch.hpp"
#include <iostream>


void display(ostream &os, const circle &c) {
    os << "circle{center: (" << c.x << ", " << c.y << "), radius: " << c.radius << "}";
}

void display(ostream &os, const square &s) {
    os << "square{corner: (" << s.x1 << ", " << s.y1 << "), width: " << c.width << "}";
}

void display(ostream &os, const rectangle &r) {
    os << "rectangle{(" << r.x1 << ", " << r.y1 << ")-(" << r.x2 << ", " << r.y2 << ")}";
}


std::ostream &operator<<(std::ostream &os, const shape &s) {
    multi_dispatch<shapes>([os](auto &s) { display(os, s); }, s);
    return os;
}

