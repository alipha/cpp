#include <iostream>
#include <utility>
#include <mutex>
#include <boost/type_index.hpp>
#include <source_location>
#include <string>

struct A {
    int i;

    int update(int j) {
        i = j;
        return i;
    }

    int get() {
        return i;
    }
};

template<auto Func>
struct fn {};

template<typename Ret, typename Class, typename... As, Ret(Class::*Func)(As...)>
struct fn<Func> {
    template<typename... Args>
    fn(Args&&... args) {
        std::string name = __PRETTY_FUNCTION__;
        func = [name, t=std::make_tuple(std::forward<decltype(args)>(args)...)](Class &obj) {
            std::cout << "Invokation of " << name.substr(name.find('[')) << "\n";
            auto r = std::apply(Func, std::tuple_cat(std::tuple(&obj), t));
            std::cout << "Done" <<"\n";
            return r;
        };
    }

    std::function<Ret(Class&)> func;
};

template <typename T>
struct Audited {
    T object;

    template <auto Func>
    auto operator->*(fn<Func> c) {
        return c.func(object);
    }
};

int
main() {
    Audited<A> a{{5}};
    a->*fn<&A::update>(3);
    std::cout << a->*fn<&A::get>();
}
