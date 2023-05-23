#include <type_traits>
#include <vector>
#include <list>
#include <deque>
#include <iostream>


template<typename T, template<typename...> typename Template>
constexpr bool is_template_v = false;

template<template<typename...> typename Template, typename... Args>
constexpr bool is_template_v<Template<Args...>, Template> = true;


template<typename T, template<typename...> typename Template>
concept is_template = is_template_v<T, Template>;



template<typename T>
struct foo {
    static void f() { std::cout << "generic T\n"; }
};

template<typename T> requires(is_template<T, std::vector> || is_template<T, std::list>)
struct foo<T> {
    static void f() { std::cout << "vector or list\n"; }
};



int main() {
    foo<std::vector<int>>::f();
    foo<std::list<double>>::f();
    foo<std::deque<int>>::f();
    foo<char>::f();
}

