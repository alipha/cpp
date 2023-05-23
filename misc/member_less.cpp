#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>


struct mystruct {
    int foo;
    int bar;
    std::string quux;
};

template<auto MemPtr>           // `auto` template params are c++17
struct member_less {
    template<typename T>
    bool operator()(const T &left, const T &right) const {
        return left.*MemPtr < right.*MemPtr;
    }
};


void print(const std::vector<mystruct> &v) {
    std::cout << "mystruct:\n";
    for(auto &s : v) {
        std::cout << s.foo << '\t' << s.bar << '\t' << s.quux << std::endl;
    }
}


int main() {
    std::vector<mystruct> v{
        {3, 10, "hello"},
        {6, 8, "bye"},
        {4, 9, "test"},
        {9, 1, "apple"}
    };

    std::sort(v.begin(), v.end(), member_less<&mystruct::foo>{});
    print(v);

    std::sort(v.begin(), v.end(), member_less<&mystruct::bar>{});
    print(v);

    std::sort(v.begin(), v.end(), member_less<&mystruct::quux>{});
    print(v);
}

