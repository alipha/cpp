#include <cstdio>
#include <memory>


struct fclose_deleter {
    void operator()(FILE *f) { std::fclose(f); }
};


// or you can make a generic deleter which wraps a non-member function (c++17)
template<auto Deleter>
struct deleter {
    template<typename T>
    void operator()(T *p) { Deleter(p); }
};


int main() {
    std::unique_ptr<FILE, fclose_deleter> fp(std::fopen("foo", "rb"));

    std::unique_ptr<FILE, deleter<std::fclose>> fp2(std::fopen("foo", "rb"));
}
