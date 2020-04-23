#include "function_traits.hpp"
#include <iostream>

using namespace liph;


struct bar {
    void const_something() const;
    void something() &;
};

std::string foo(int) {
    function_return_type<foo> test = "hello";
    return test;
}


int main() {
    std::cout << std::is_same_v<function_class_type<foo>, void> << std::endl;
    std::cout << is_member_function<foo> << std::endl;
    std::cout << is_member_function<&bar::const_something> << std::endl;
    std::cout << is_const_member_function<foo> << std::endl;
    std::cout << is_const_member_function<&bar::const_something> << std::endl;
    std::cout << is_const_member_function<&bar::something> << std::endl;
    std::cout << is_lvalue_member_function<&bar::something> << std::endl;
}
