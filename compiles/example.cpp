#include "compiles.hpp"
#include <iostream>
#include <string>
#include <tuple>


// 

template<typename T>
void test1(const T &x) {
    // LIPH_ARG needs to wrap the template parameters
    if constexpr(LIPH_COMPILES(T, LIPH_ARG(x) / 5)) {
        std::cout << "can divide\n";
    } else {
        std::cout << "no / operator\n";
    }
}

template<typename T>
void test2() {
    // if you don't have an object of the template type
    if constexpr(LIPH_COMPILES(T, *LIPH_DECLVAL(T))) {
        std::cout << "dereferenceable\n";
    } else {
        std::cout << "no dereference operator\n";
    }
}

template<typename T, typename U>
void test3(const T &x, const U &y) {
    // only one of the template arguments needs to be provided.
    // I picked T; U could have been used instead.
    // it doesn't matter which one; it just needs to be one of arguments to
    // the function itself (ie, not a "fixed template argument")
    if constexpr(LIPH_COMPILES(T, LIPH_ARG(x) + y)) {  
        std::cout << "can add\n";
    } else {
        std::cout << "no + operator\n";
    }
}


template<typename T>
class foo {
public:
    // don't need this below trick to delay evaluation of the template parameter
    // template<typename U = T>
    void test4() {
        if constexpr(LIPH_COMPILES(T, std::cout << LIPH_ARG(x))) {
            std::cout << "foo::x has stream output\n";
        } else {
            std::cout << "no << operator\n";
        }
    }

    T x;
};


template<typename... Args>
void test5(Args&&... args) {
    // parameter packs can be used
    if constexpr(LIPH_COMPILES(Args..., ((std::cout << LIPH_ARG(args)), ...))) {
        std::cout << "each argument has <<\n";
    } else {
        std::cout << "not all arguments have <<\n";
    }
}


template<typename... Args>
void test6(const std::tuple<Args...> &) {
    // parameter packs can be used
    if constexpr(LIPH_COMPILES(Args..., ((std::cout << LIPH_DECLVAL(Args)), ...))) {
        std::cout << "each element in the tuple has <<\n";
    } else {
        std::cout << "not all elements in tuple have <<\n";
    }
}


template<typename T>
void test7() {
    if constexpr(LIPH_HAS_MEMBER(T, size)) {
        std::cout << "T::size exists\n";
    } else {
        std::cout << "T::size does not exist\n";
    }
}


template<typename T>
void test8() {
    if constexpr(LIPH_HAS_TYPE_MEMBER(T, value_type)) {
        std::cout << "T::value_type exists\n";
    } else {
        std::cout << "T::value_type does not exist\n";
    }
}


// works in c++20 (but then I'm not sure why you wouldn't just use concepts)
//template<typename T>
//std::enable_if_t<LIPH_COMPILES(T, LIPH_DECLVAL(T) / 5)> bar() {}


struct test_struct {};


int main() {
    test1(3);
    test1("foo");
    test2<int>();
    test2<int*>();
    test3(5, 2.3);
    test3("foo", "bar");
    
    foo<int> f;
    f.test4();

    foo<test_struct> f2;
    f2.test4();

    test5(2, 12.0, "test");
    test5(2, test_struct());

    test6(std::tuple<int, double, const char*>{2, 12.0, "test"});
    test6(std::tuple<int, test_struct>());
    
    test7<std::tuple<int>>();
    test7<std::string>();

    test8<std::tuple<int>>();
    test8<std::string>();

    //bar<int>();
}
