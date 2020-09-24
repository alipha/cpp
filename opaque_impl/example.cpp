#include "rect.h"
#include <iostream>

int main() {
    rect r(3, 5);
    std::cout << r.area() << std::endl;
    r.set_width(4);
    std::cout << r.area() << std::endl;
}
