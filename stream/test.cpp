#include "basic.hpp"
#include <iostream>


using namespace stream;


int main() {
//    for(int x : range(10, 20) | filter([](int x) { return x % 2; })) {
//        std::cout << x << std::endl;
//    }

    std::vector<int> result = range(10, 49) 
        | mapping([](auto &&x) { return x / 3; }) 
        | adj_unique 
        | filter([](auto &&x) { return x % 2; })
        | as<std::vector<int>>;
//        | as_vector;

   for(int x : result)
      std::cout << x << std::endl; 
}
