#include "any_iterator.hpp"
#include "old_any_iterator.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <ctime>
#include <cstdlib>


template<typename It>
void sort_timing() {
    std::vector<int> v;
    for(int i = 0; i < 10000000; ++i)
        v.push_back(std::rand());

    std::clock_t start = std::clock();

    It first = v.begin();
    It last = v.end();
    std::sort(first, last);

    std::clock_t end = std::clock();
    std::cout << 1000.0 * (end - start) / CLOCKS_PER_SEC << std::endl;

    int max = v[0];
    for(int next : v) {
        if(next < max) {
            std::cout << "out of order" << std::endl;
            return;
        }
    }
}



int main() {
    std::srand(std::time(nullptr));

    std::cout << sizeof(liph::any_random_access_iterator<int>) << std::endl;

    std::vector<int> v{5, 2, 10, 15};
    liph::any_input_iterator<int> it, it2 = v.begin(), it3;
    it = it2;
    it3 = v.end();

    for(; it2 != it3; ++it2)
        std::cout << *it2 << std::endl;

    //auto it4 = it++;

    while(!(it == it3))
        std::cout << *it++ << std::endl;

    liph::any_random_access_iterator<int> rit = v.begin(), rit2;
    rit2 = v.end();
    auto rit3 = rit2;
    liph::any_random_access_iterator<int> rit4;
    rit4 = rit2;
    rit4 = rit + 3;

    //for(; rit != rit2; ++rit)
    //    std::cout << *rit << std::endl;

    std::sort(rit, rit2);
    for(int i = 1; i <= 4; ++i)
        std::cout << *(rit3 - i) << std::endl;


    sort_timing<std::vector<int>::iterator>();
    sort_timing<liph::any_random_access_iterator<int>>();
    sort_timing<old::liph::any_random_access_iterator<int>>();
    return rit3 < rit4;
}
