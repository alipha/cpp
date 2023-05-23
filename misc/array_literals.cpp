#include <iostream>
#include <array>

// each lambda is a unique class, so each of these static arrays should be unique
#define MAKE_LITERAL(...) (([]() { static std::array values{__VA_ARGS__}; return values.data(); })())


struct Foo {
    ~Foo() { std::cout << "~Foo"; }
};


void test(const int *p) {
    std::cout << p[0] << p[1] << p[2] << '\n';
}


int main() {
    const int *ip = MAKE_LITERAL(3, 4, 5);
    test(MAKE_LITERAL(5, 2, 1));
    test(ip);
    
    const Foo *p = MAKE_LITERAL(Foo(), Foo());
    std::cout << "still alive\n";
}

