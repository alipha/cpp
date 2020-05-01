#include "cached_function.hpp"

#include <iostream>
#include <string>


std::string bar_array[] = {"zero", "one", "two"};

// you probably want to use cached_function instead of basic_cached_function
// basic_cached_function only caches the previous call.
// cached_function caches (by default) the previous 5 calls.
basic_cached_function foo = [](int x, int y) { 
    std::cout << "foo called: " << x + y << std::endl;
    return x + y; 
};

basic_cached_function bar = [](const int &i) -> std::string& {
    std::cout << "bar called: " << bar_array[i] << std::endl;
    return bar_array[i];
};

basic_cached_function baz = [](int *i) -> std::string& {
    std::cout << "baz called: " << bar_array[*i] << std::endl;
    return bar_array[*i];
};


int main() {
    std::cout << foo(3, 4) << std::endl;
    std::cout << foo(5, 4) << std::endl;
    std::cout << foo(3, 4) << std::endl;
    std::cout << foo(3, 4) << std::endl;
    std::cout << bar(2) << std::endl;
    int i = 1;
    std::cout << bar(i) << std::endl;
    std::cout << baz(&i) << std::endl;
    bar(1) = "ONE";
    std::cout << bar(i) << std::endl;
    std::cout << baz(&i) << std::endl;
    i = 0;
    std::cout << bar(i) << std::endl;
    std::cout << baz(&i) << std::endl;
    return 0;
}
