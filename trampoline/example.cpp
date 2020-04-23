#include "trampoline.hpp"
#include <iostream>

using liph::trampoline;



trampoline<int> fibonacci(int n) {
    if(n <= 1) {
        return 1;
    } else {
        auto result1 = [n]() { return fibonacci(n - 1); };
        auto result2 = [n]() { return fibonacci(n - 2); };
        return trampoline([](int n1, int n2) { return n1 + n2; }, result1, result2);
        /* instead of:
        auto n1 = fibonacci(n - 1);
        auto n2 = fibonacci(n - 2);
        return n1 + n2;
        */
    }
}



trampoline<int> factorial(int n) {
    if(n <= 1) {
        return 1;
    } else {
        auto result1 = [n]() { return factorial(n - 1); };
        return trampoline([n](int n1) { return n * n1; }, result1);
        /* instead of:
        n1 = factorial(n - 1);
        return n * n1;
        */
    }
}



trampoline<long long> sum(long long n) {
    if(n <= 1) {
        return 1;
    } else {
        auto result1 = [n]() { return sum(n - 1); };
        return trampoline([n](long long n1) { return n + n1; }, result1);
    }
}

long long sum_recursive(long long n) {
    if(n <= 1)
        return 1;
    else
        return n + sum_recursive(n - 1);
}



int main() {
    std::cout << fibonacci(10).run() << std::endl;
    std::cout << factorial(10).run() << std::endl;
    
    std::cout << sum(1000000).run() << std::endl;
    std::cout << "recursive: " << std::endl;
    std::cout << sum_recursive(1000000) << std::endl;
}

