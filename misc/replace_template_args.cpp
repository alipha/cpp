#include <vector>
#include <iostream>
#include <deque>


template<typename T, typename... NewArgs>
struct replace_template_args {
    static_assert(sizeof(T) && false, "T is not a template");
};

template<template<typename...> typename Template, typename... Args, typename... NewArgs>
struct replace_template_args<Template<Args...>, NewArgs...> {
    using type = Template<NewArgs...>;
};

template<typename T, typename... NewArgs>
using replace_template_args_t = typename replace_template_args<T, NewArgs...>::type;



template<typename T, template<typename...> typename Template>
struct replace_template {
    static_assert(sizeof(T) && false, "T is not a template");
};

template<template<typename...> typename OldTemplate, template<typename...> typename NewTemplate, typename... Args>
struct replace_template<OldTemplate<Args...>, NewTemplate> {
    using type = NewTemplate<Args...>;
};

template<typename T, template<typename...> typename Template>
using replace_template_t = typename replace_template<T, Template>::type;



int main() {
    replace_template_args_t<std::vector<int>, double> v{3.5, 2.1, 6.7};
    replace_template_t<std::vector<int>, std::deque> d{3, 2, 6};
    std::cout << v[0];
    d.pop_front();
}

