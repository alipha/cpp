#include "callback_factory.hpp"
#include <iostream>
#include <iomanip>
#include <string>



struct quux {
    virtual ~quux() {}

    virtual void test3() { std::cout << "quux" << std::endl; }
    std::string foo;
};

struct bar {
    bar(std::string name) : name(name) {}
    virtual ~bar() {}

    void test() { std::cout << "hello from " << name << std::endl; }

    virtual void test2() const { std::cout << "virtual hello from " << name << std::endl; }

    int test4(int x, double y, char z) {
        std::cout << "\ntest3: " << name << x << ' ' << y << ' ' << z << std::endl;
        return x * 2;
    }

    int test5(int x, int y) {
        return x + y;
    }


    std::string name;
};

struct baz : bar, quux {
    using bar::bar;

    void test2() const override { std::cout << "overridden hello from " << name << "\n"; }

    int y;
};


void print(void (*f)()) {
    unsigned char *p = (unsigned char*)f;
    for(int i = 0; i < 48; ++i)
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)p[i] << ' ';
    std::cout << std::endl;
}


int main() {
    std::cout << sizeof(&bar::test) << ' ' << sizeof(&bar::test2) << ' ' << sizeof(void(*)()) << '\n';
    bar b{"Bob"};
    bar c{"Chris"};
    baz d{"Dan"};

    auto mp = &baz::test3;
    void (baz::*bp)() = mp;

    liph::callback_factory f;
    void (*p1)() = f.make_raw_callback(&b, &bar::test);
    void (*p2)() = f.make_raw_callback(c, &bar::test);
    void (*p3)() = f.make_raw_callback(&c, &bar::test2);
    void (*p4)() = f.make_raw_callback(&d, &baz::test2);    // TODO: work with bar::test2
    void (*p5)() = f.make_raw_callback(&d, &bar::test2);
    void (*p6)() = f.make_raw_callback(&d, bp);
    int (*p7)(int, double, char) = f.make_raw_callback(&b, &bar::test4);
    int (*p8)(int, int) = f.make_raw_callback(&b, &bar::test5);
    std::cout << std::dec;
    p1();
    p2();
    p3();
    p4();
    p5();
    p6();
    int result = p7(10, 3.2, 'c');
    std::cout << "p7= " << result << std::endl;
    std::cout << "p8= " << p8(55, 2200) << std::endl;
}

