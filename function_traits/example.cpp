#include "function_traits.hpp"
#include <iostream>

using namespace liph;


struct bar {
    void const_something() const;
    void something() &;
};

std::string foo(int) {
    function_return_type<decltype(foo)> test = "hello";
    return test;
}


int main() {
    std::cout << std::is_same_v<function_class_type<decltype(foo)>, void> << std::endl;
    std::cout << is_member_function<decltype(foo)> << std::endl;
    std::cout << is_member_function<decltype(&bar::const_something)> << std::endl;
    std::cout << is_const_member_function<decltype(foo)> << std::endl;
    std::cout << is_const_member_function<decltype(&bar::const_something)> << std::endl;
    std::cout << is_const_member_function<decltype(&bar::something)> << std::endl;
    std::cout << is_lvalue_member_function<decltype(&bar::something)> << std::endl;
}
