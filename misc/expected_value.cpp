#include <iostream>
#include <expected>
#include <utility>

class C {
	public:
    friend class std::expected<C, int>;
    friend std::expected<C, int> func(int);
    C() = default;
    C(const C&) = delete;
    C(C&&) = delete;
public:
    ~C() = default;
};

template<typename... Args>
struct expected_value {
	template<typename... CArgs>
	expected_value(CArgs&&... args) : args(std::forward<CArgs>(args)...) {}

	template<typename T, typename U>
	operator std::expected<T, U>() const { 
		return std::apply([](auto&&... args) { 
			return std::expected<T, U>(std::in_place, std::forward<decltype(args)>(args)...);
		}, args); 
	}

private:
	std::tuple<Args...> args;
};

template<typename... CArgs>
expected_value(CArgs&&...) -> expected_value<CArgs&&...>;



std::expected<C, int> func(int i) {
    if(i == 0) return expected_value();
    return std::unexpected(i);
}

std::expected<std::string, int> func2(const char *p) {
	if(p) return expected_value(p, 3);
	return std::unexpected(-1);
}

int main() {
    auto e = func(5);
    std::cout << e.error();

	auto e2 = func2("hello");
	std::cout << e2.value();
}

