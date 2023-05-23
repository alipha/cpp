#include <cstdint>
#include <iostream>
#include <string>

struct moo{};

template<typename T>
struct hw_addr : moo {
    explicit constexpr hw_addr(std::uintptr_t p) : ptr(p) {}

    T* get() const { return reinterpret_cast<T*>(ptr); }

    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }
    operator T*() const { return get(); }

    // maybe?
    T** operator&() { return reinterpret_cast<T**>(&ptr); }
    T* const* operator&() const { return reinterpret_cast<T* const*>(&ptr); }

    std::uintptr_t ptr;
};


constexpr hw_addr<int> test{0x12346526};
constexpr hw_addr<std::string> test2{0xDEADBEEF};


template<hw_addr<int> Addr>
void foo() {
    std::cout << "The address is: " << Addr << std::endl;
    int *p = Addr;
    int x = *Addr;  // -O2 optimizes this out, hence no segfault
}


int main() {
    foo<test>();
    foo<hw_addr<int>{0x523}>();
    test2->size();
    int *const *pp = &test;
}

