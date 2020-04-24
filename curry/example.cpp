#include "curry.hpp"
#include <iostream>


int add(int a, int b, int c) { 
    return a + b + c;
}


int main()
{
    auto f = liph::curry(add);
    std::cout << f(1, 6.2)(2) << std::endl; 
    std::cout << f(1)(6.2)(2) << std::endl; 
    
    int x = 5;
    auto g = liph::curry([x](auto a, auto b, auto c) { return a + b + c + x; });
    std::cout << g(3.2)(6, 1) << std::endl;
    std::cout << g(3.2)(6)(1) << std::endl;
}

