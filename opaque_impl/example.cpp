#include "rect.h"
#include <iostream>

int main() {
    rect r(3, 5);
    rect s(r);
    std::cout << r.area() << ' ' << s.area() << std::endl;
    r.set_width(4);
    std::cout << r.area() << ' ' << s.area() << std::endl;
    s = r;
    std::cout << r.area() << ' ' << s.area() << std::endl;
}
