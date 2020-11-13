#include <iostream>
#include "precompute.h"


int main () { 
    constexpr liph::precompute<int, 10> square([](int x) { return x * x; });
    char test[square(5)];
    std::cout << sizeof test << std::endl;
    
    constexpr liph::precompute<int, 10, 8> multiply([](int x, int y) { return x * y; });
    char test2[multiply(4, 5)];
    std::cout << sizeof test2 << std::endl;
    
    return 0;
}

