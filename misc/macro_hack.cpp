#include <iostream>


#define Child(...) EVAL(WHICH_ARGS_CHILD, NO_ARGS_CHILD __VA_ARGS__(), (__VA_ARGS__),)
#define REMOVE_FIRST_ARG(x, ...) __VA_ARGS__
#define WHICH_ARGS_CHILD(x, y, z, ...) z EVAL2(REMOVE_FIRST_ARG, HAS_ARGS_CHILD y)
#define EVAL(x, ...) x(__VA_ARGS__)
#define EVAL2(x, ...) x(__VA_ARGS__)
#define HAS_ARGS_CHILD(...) , template<typename... Args> Child(Args&&... args) : Base(std::forward<Args>(args)...)
#define NO_ARGS_CHILD(...) 0,, Child()


class Base {
public:
    Base(int x_, int y_) : x(x_), y(y_) {}

    int x;
    int y;
};


class Child : public Base {
public:
    Child(int x) {}
    ~Child() {}
};


int main() {
    Child c(3, 5);
    std::cout << c.x << ' ' << c.y << '\n';
}
