#include "basic.hpp"
#include <iostream>


using namespace stream;


int main() {
    std::vector<int> result = range(10, 49) 
        | mapping([](int x) { return x / 3; }) 
        | adj_unique 
        | filter([](int x) { return x % 2; })
        | as_vector;

   for(int x : result)
      std::cout << x << std::endl; 
}
