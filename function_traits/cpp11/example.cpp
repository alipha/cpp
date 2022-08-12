#include "function_traits.hpp"
#include <iostream>
#include <string>

using namespace liph;


struct bar {
    void const_something() const;
    void something() &;
};

std::string foo(int) {
    typename function_traits<decltype(foo)>::return_type test = "hello";
    return test;
}


int main() {
    // note: class_type is possibly const-qualified or ref-qualified based upon the if the member function is qualified
    std::cout << std::is_same<typename function_traits<decltype(foo)>::class_type, void>::value << std::endl;   // true 
    std::cout << std::is_same<typename function_argument_n<0, decltype(foo)>::type, int>::value << std::endl;   // true
    std::cout << function_traits<decltype(foo)>::is_member << std::endl;                                        // false
    std::cout << function_traits<decltype(&bar::const_something)>::is_member << std::endl;                      // true
    std::cout << function_traits<decltype(foo)>::is_const << std::endl;                                         // false
    std::cout << function_traits<decltype(&bar::const_something)>::is_const << std::endl;                       // true
    std::cout << function_traits<decltype(&bar::something)>::is_const << std::endl;                             // false
    std::cout << function_traits<decltype(&bar::something)>::is_lvalue_ref << std::endl;                        // true
    std::cout << std::endl;

    using func_ptr = int(*)(std::string, double);
    using mem_func_ptr = int(bar::*)(std::string, double);
    using const_rvalue_func_ptr = int(bar::*)(std::string, double) const &&;

    std::cout << std::is_same<typename make_member_function_pointer<bar, func_ptr>::type, mem_func_ptr>::value << std::endl;                        // true
    std::cout << std::is_same<typename make_const_rvalue_member_function_pointer<bar, func_ptr>::type, const_rvalue_func_ptr>::value << std::endl;  // true
    std::cout << std::is_same<typename make_function_pointer<mem_func_ptr>::type, func_ptr>::value << std::endl;                                    // true
    std::cout << std::is_same<typename make_function_pointer<const_rvalue_func_ptr>::type, func_ptr>::value << std::endl;                           // true
}
