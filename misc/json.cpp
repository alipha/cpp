#include <variant>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <map>
#include <concepts>
#include <cstddef>
#include <sstream>
#include <iostream>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;


struct Object {
	template<std::size_t N>
	Object(const char (&x)[N]) : value(std::string(x)) {}

	Object(std::integral auto x) : value(x) {}			// if you want c++17 (or older), use std::enable_if_t instead of concepts
	Object(double x) : value(x) {}
	Object(std::string x) : value(std::move(x)) {}
	Object(std::initializer_list<std::pair<std::string, Object>> x) : value(std::map<std::string, Object>(x.begin(), x.end())) {}
	Object(std::vector<Object> x) : value(x) {}

	std::string to_json(int indent = 0) const {
		return std::visit(overloaded {
			[](std::int64_t x) { return std::to_string(x); },
			[](double x) { std::stringstream ss; ss << x; return std::move(ss).str(); },
			[](std::string x) { return '"' + x + '"'; },	// todo: escape " and other characters
			[&](const std::vector<Object> &v) {
				std::string output;
				output += "[ ";
				for(auto &obj : v) {
					output += '\n' + std::string(indent + 1, '\t') + obj.to_json(indent + 1) + ',';
				}
				output.pop_back();  // remove extra ,
				output += '\n' + std::string(indent, '\t') + ']';
				return output;
			},
			[&](const std::map<std::string, Object> &m) { 
				std::string output;
				output += "{ ";
				for(auto &kv : m) {
					output += '\n' + std::string(indent + 1, '\t') + '"' + kv.first + "\": " + kv.second.to_json(indent + 1) + ',';
				}
				output.pop_back();  // remove extra ,
				output += '\n' + std::string(indent, '\t') + '}';
				return output;
			}
		}, value);
	}

	std::variant<std::int64_t, double, std::string, std::vector<Object>, std::map<std::string, Object>> value; 
};

using A = std::vector<Object>;		// array

int main() {
	Object obj = {
		{"moo", "cow"},
		{"abc", 3},
		{"foo", 5.1},
		{"bar", A{
			"test",
			9,
			{{"test2", A{3, 5, 1}}},
			{{"test3", {{"test4", "moo"}}}}
		}}
	};

	std::cout << obj.to_json() << std::endl;
}
