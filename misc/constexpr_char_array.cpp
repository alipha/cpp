#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <algorithm>

template<std::size_t N>
struct char_array {
    char storage[N+1];
    std::size_t len;
    
    constexpr char_array(std::string_view str) : storage{}, len{str.size()} {
        if(str.size() > N) {
            throw std::logic_error("Initializing char_array<" + std::to_string(N) + "> with " 
                + std::to_string(str.size()) + " characters");
        }
        std::copy(str.begin(), str.end(), storage);
    }

    constexpr std::size_t size() const { return len; }

    constexpr const char *begin() const { return storage; }
    constexpr const char *end() const { return storage + len; }
    constexpr const char *data() const { return storage; }

    constexpr char *begin() { return storage; }
    constexpr char *end() { return storage + len; }
    constexpr char *data() { return storage; }

    constexpr std::string str() const { return std::string(storage, storage + len); }
    constexpr std::string_view view() const { return std::string_view(storage, storage + len); }
};

template<typename StrFunc>
constexpr auto constexpr_char_array(StrFunc func) {
    return char_array<std::string(func()).size()>{func()};
}



template<std::size_t N, std::size_t Size>
constexpr auto repeat_n(const char (&str)[Size]) {
    std::string ret = "";
    for(int i = 0; i < N; ++i) {
        ret += str;
    }
    // should be `N * (Size - 1)` for optimal storage
    // i'm just doing `N * 100` to "test out" having a char_array::len < char_array's storage
    return char_array<N * 100>(ret);
}

template<std::size_t N>
constexpr auto test_string() {
    // automatically computes the size of the char_array, unlike repeat_n above.
    // repeat_n can't use constexpr_char_array though because it takes a function parameter
    return constexpr_char_array([]() constexpr {
        std::string ret = "";
        for(int i = 1; i <= N; ++i) {
            ret += std::string(i, '0' + i);
        }
        return ret;
    });
}

int main()
{
    // x is char_array<300>, but could be char_array<15> if `N * (Size - 1)` were used 
    constexpr auto x = repeat_n<3>("hello");
    std::cout << sizeof x << std::endl;
    std::cout << x.size() << std::endl;
    std::cout << x.view() << std::endl;

    constexpr auto t = test_string<6>();
    std::cout << t.size() << std::endl;
    std::cout << t.view() << std::endl;
}

