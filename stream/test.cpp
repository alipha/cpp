#include "basic.hpp"
#include "stream.hpp"
#include <iostream>


using namespace stream;


int main() {
    for(int x : range(10, 20) | filter([](int x) { return x % 2; })) {
        std::cout << x << std::endl;
    }

    std::vector<int> result = range(10, 49) 
        | mapping([](auto &&x) { return x / 3; }) 
        | adj_unique 
        | filter([](auto &&x) { return x % 2; })
        | as<std::vector<int>>;
//        | as_vector;

    streamer result2 = result 
        | mapping([](auto &&x) { return x * 2; });

    streamer result3 = range(50, 53);
    streamer result4 = result;

//    auto it = result2.begin();
//    for(int i = 0; i < 15; ++i, ++it)
//        std::cout << *it << ' ' << (it == result2.end()) << std::endl;
    for(int x : result | mapping([](auto &&x) { return x * 2; }))
        std::cout << x << std::endl; 
}
