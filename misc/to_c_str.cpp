#include <type_traits>
#include <charconv>
#include <limits>
#include <iterator>
#include <iostream>


template<typename T>
struct to_c_str {
    // i didn't want to figure out how to calculate the space needed for a float
    static_assert(std::is_integral_v<T>);

    to_c_str(T value) {
        auto [ptr, ec] = std::to_chars(std::begin(str), std::end(str) - 1, value);
        *ptr = '\0';
    }

    operator const char *() const { return str; }

    char str[std::numeric_limits<T>::digits10 + 3];
};


int main() {
    std::cout << "Value: '" << to_c_str(-125) << "'\n";
}

