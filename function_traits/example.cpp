#include "function_traits.hpp"
#include <iostream>
#include <string>

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
    std::cout << std::is_same_v<function_argument_n<0, decltype(foo)>, int> << std::endl;
    std::cout << is_member_function<decltype(foo)> << std::endl;
    std::cout << is_member_function<decltype(&bar::const_something)> << std::endl;
    std::cout << is_const_member_function<decltype(foo)> << std::endl;
    std::cout << is_const_member_function<decltype(&bar::const_something)> << std::endl;
    std::cout << is_const_member_function<decltype(&bar::something)> << std::endl;
    std::cout << is_lvalue_member_function<decltype(&bar::something)> << std::endl;
    std::cout << std::endl;

    using func_ptr = int(*)(std::string, double);
    using mem_func_ptr = int(bar::*)(std::string, double);
    using const_rvalue_func_ptr = int(bar::*)(std::string, double) const &&;

    std::cout << std::is_same_v<make_member_function_pointer<bar, func_ptr>, mem_func_ptr> << std::endl;
    std::cout << std::is_same_v<make_const_rvalue_member_function_pointer<bar, func_ptr>, const_rvalue_func_ptr> << std::endl;
    std::cout << std::is_same_v<make_function_pointer<mem_func_ptr>, func_ptr> << std::endl;
    std::cout << std::is_same_v<make_function_pointer<const_rvalue_func_ptr>, func_ptr> << std::endl;
}
