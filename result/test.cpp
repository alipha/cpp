#include "result.hpp"

#include <charconv>
#include <iostream>
#include <string>



template<typename T>
liph::result<T, std::errc> parse_chars(const char *first, const char *last) {
    T value;
    auto [p, ec] = std::from_chars(first, last, value);
    if(ec == std::errc())
        return value;
    else
        return ec;
}


void test(std::string input) {
    std::cout << "testing " << input << std::endl;
    liph::result<int, std::errc> r = parse_chars<int>(input.data(), input.data() + input.size());
    
    const auto cr = liph::result<double, std::errc>(r);
    
    if(cr.has_value())
        std::cout << "has value: " << cr.value() << std::endl;
    if(cr.has_error())
        std::cout << "has error: " << (cr.error() == std::errc::invalid_argument) << std::endl;
    if(cr)
        std::cout << "has value (bool check)" << std::endl;
    
    std::cout << "value_or(2.5): " << cr.value_or(2.5) << std::endl;

    double x = cr.value_or_get([](std::errc e) { 
        std::cout << "inside value_or_get" << std::endl;
        return e == std::errc::invalid_argument; 
    });

    std::cout << "value_or_get: " << x << std::endl;

    try {
        std::cout << "value_or_throw (const): " << cr.value_or_throw() << std::endl;
    } catch(const std::errc &e) {
        std::cout << "caught: " << (e == std::errc::invalid_argument) << std::endl;
    }

    try {
        std::cout << "value_or_throw: " << r.value_or_throw() << std::endl;
    } catch(std::errc &e) {
        std::cout << "caught: " << (e == std::errc::invalid_argument) << std::endl;
    }

    try {
        std::cout << "value_or_throw<> (const): " << cr.value_or_throw<std::runtime_error>("thrown") << std::endl;
    } catch(const std::runtime_error &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }

    try {
        std::cout << "value_or_throw<>: " << r.value_or_throw<std::runtime_error>("thrown2") << std::endl;
    } catch(std::runtime_error &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }

    cr.perform(
        [](double x) { 
            std::cout << "perform value (const): " << x << std::endl; 
        },
        [](std::errc e) { 
            std::cout << "perform error (const): " << (e == std::errc::invalid_argument) << std::endl;
        }
    );

    r.perform(
        [](int &x) { 
            x = 200; 
            std::cout << "perform value" << std::endl; 
        },
        [](std::errc &e) { 
            e = std::errc::address_in_use;
            std::cout << "perform error" << std::endl;
        }
    );

    liph::result<double, std::errc> r2 = r.map([](int x) {
        std::cout << "inside map: " << x << std::endl;
        return x / 3.0; 
    });
    std::cout << "r2: " << r2.value_or(-999) << std::endl;

    liph::result<double, bool> r3 = r.map(
        [](int x) {
            std::cout << "inside map (value): " << x << std::endl;
            return x / 400.0;
        },
        [](std::errc e) {
            bool ret = (e == std::errc::address_in_use); 
            std::cout << "inside map (error): " << ret << std::endl;
            return ret;
        }
    );
    std::cout << "r3: " << r3.value_or(-999) << std::endl;

    try {
        std::cout << "before value: " << r.value() << std::endl;
        r.value() = 3;
        std::cout << "updated value: " << r.value() << std::endl;
    } catch(liph::bad_result_value_access &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }

    try {
        std::cout << "before error: " << (r.error() == std::errc::address_in_use) << std::endl;
        r.error() = std::errc::result_out_of_range;
        std::cout << "updated error: " << (r.error() == std::errc::result_out_of_range) << std::endl;
    } catch(liph::bad_result_error_access &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }
}


int main() {
    test("42");
    test("abc");
    
    liph::result<std::string, std::runtime_error> r(liph::result_in_place_value(), 5, 'A');
    std::cout << "AAAAA: " << r.value() << std::endl;
    r = "foo";
    std::cout << "foo: " << r.value() << std::endl;
    r = liph::result<std::string, std::runtime_error>(liph::result_in_place_error(), "test");
    std::cout << "test: " << r.error().what() << std::endl;
    r = std::runtime_error("test2");
    std::cout << "test2: " << r.error().what() << std::endl;
    
    r = liph::result<std::string, std::runtime_error>(liph::result_in_place_value(), {'h', 'e', 'y'});
    std::cout << "hey: " << r.value() << std::endl;
    
    std::cout << "hello: " << r.emplace_error("hello").what() << std::endl;
    try {
        std::cout << r.value_or_throw();
    } catch(std::runtime_error &e) {
        std::cout << "caught: " << e.what() << std::endl;
    }

    r.emplace_value(3, 'B').append("bar");
    std::cout << "BBBbar: " << r.value() << std::endl;
}


