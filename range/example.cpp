#include "range.h"

#include <iostream>
#include <string>


int main() {
    using namespace std::string_literals;
    
    for(int x : range(4, 10))
        std::cout << x << " ";
    std::cout << std::endl;
    
    for(int x : range(8, 2, decrement()))
        std::cout << x << " ";
    std::cout << std::endl;
    
    for(std::string x : range("a"s, "aaaaa"s, [](auto &&x) { x += 'a'; }))
        std::cout << x << " ";
    std::cout << std::endl;
}

