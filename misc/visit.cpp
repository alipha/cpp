
// credit mostly to mort with some improvements by Alipha

#include <variant>
#include <tuple>
#include <utility>
#include <iostream>
 
// visit.h

template<typename... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template<typename... Variants>
class VisitorHelper {
public:
	VisitorHelper(Variants&&... vs): vs_{std::forward<Variants>(vs)...} {}

	decltype(auto) operator->*(auto &&visitor) {
		return std::apply([&](auto&&... vs) {
			return std::visit(std::forward<decltype(visitor)>(visitor), std::forward<decltype(vs)>(vs)...);
		}, std::move(vs_));
	}

private:
	std::tuple<Variants&&...> vs_;
};

template<typename... Variants>
VisitorHelper(Variants&&...) -> VisitorHelper<Variants&&...>;


#define VISIT(...) VisitorHelper(__VA_ARGS__) ->* Overloaded	// ->* has higher precedence

// main.cc

struct FooType {
    int x;
    int y;
};

struct BarType {
    float whatever;
};

int reactTo(std::variant<FooType, BarType> v, std::variant<FooType, BarType> w) {
    return VISIT(v, std::move(w)) {
        [](FooType &foo, FooType &&foo2) {
            std::cout << "Got foo: " << foo.x << ", " << foo.y << '\n';
			std::cout << "Got foo: " << foo2.x << ", " << foo2.y << '\n';
			return 1;
        },
        [](BarType &bar, FooType &&foo) {
            std::cout << "Got bar: " << bar.whatever << '\n';
			std::cout << "Got foo: " << foo.x << ", " << foo.y << '\n';
			return 2;
        },
		[](FooType &foo, BarType &&bar) {
            std::cout << "Got foo: " << foo.x << ", " << foo.y << '\n';
			std::cout << "Got bar: " << bar.whatever << '\n';
			return 3;
        },
        [](BarType &bar, BarType &&bar2) {
            std::cout << "Got bar: " << bar.whatever << '\n';
			std::cout << "Got bar: " << bar2.whatever << '\n';
			return 4;
        },
    };
}

int main() {
    std::cout << reactTo(FooType{10, 20}, FooType{100, 200});
	std::cout << '\n';
    std::cout << reactTo(BarType{500.3}, FooType{160, 260});
}
