#include "enumerate.hpp"
#include <iostream>
#include <vector>


int main() {
    std::vector<float> v = {2.25, 3.25, 6.1};
    for(auto &[i, x] : enumerate(v))
        x *= 2;
    
    for(auto [i, x] : enumerate(v))
        std::cout << i << " = " << x << std::endl;
        
    int a[] = {2, 3, 6};
    for(auto &[i, x] : enumerate(a))
        x *= 2;
    
    for(auto [i, x] : enumerate(a))
        std::cout << i << " = " << x << std::endl;
}
