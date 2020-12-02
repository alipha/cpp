#include "functor.hpp"
#include <iostream>


int main() {
    liph::functor<int(int)> multiplier = [](auto x) { return x * 2; };
    std::cout << multiplier(8) << std::endl;
    
    int y = 3;
    multiplier = [&y](int x) { return x * y; };
    std::cout << multiplier(5) << std::endl;
}

