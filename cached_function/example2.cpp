#include "cached_function.hpp"

#include <cstdint>
#include <iostream>


std::uint64_t do_fib(std::uint64_t);


cached_function fib = do_fib;


std::uint64_t do_fib(std::uint64_t x) {
    if(x <= 1)
        return 1;
    else
        return fib(x - 1) + fib(x - 2);
}


std::uint64_t fib_no_cache(std::uint64_t x) {
    if(x <= 1)
        return 1;
    else
        return fib_no_cache(x - 1) + fib_no_cache(x - 2);
}


int main() {
    fib.set_max_size(40);
    std::cout << fib(40) << std::endl;
    std::cout << fib_no_cache(40) << std::endl;
}
