#include "switch_pack.h"

#include <iostream>
#include <string>


int main() {
    using namespace liph;

    std::string str;
    std::getline(std::cin, str);
    
    switch(pack(str)) {
        case pack(""): std::cout << "nothing" << std::endl; break;
        case pack("abc"): std::cout << "first" << std::endl; break;
        case pack("testingx"): std::cout << "second" << std::endl; break;
        case pack_overflow(): std::cout << "third" << std::endl; break;
        //case pack("too longggg"): std::cout << "error" << std::endl; break;    // compile-time error
    }
}
