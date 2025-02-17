#include <iostream>
#include <map>
#include <cstddef>
#include <optional>
#include <utility>


template<typename T>
class sparse_vector {
private:
	struct elem_proxy {
		elem_proxy() : val(T{}) {}
		elem_proxy(sparse_vector *v, std::size_t i) : vec(v), index(i) {}

		elem_proxy(const T &v) { val = v; }
		elem_proxy(T &&v)      { val = std::move(v); }

		elem_proxy(const elem_proxy &other) { val = other.value(); }

		elem_proxy &operator=(const T &v) & {
			val = v;
			return *this;
		}

		elem_proxy &operator=(T &&v) & {
			val = std::move(v);
			return *this;
		}

		elem_proxy &operator=(const T &v) && {
			value() = v;
			return *this;
		}

		elem_proxy &operator=(T &&v) && {
			std::move(*this).value() = std::move(v);
			return *this;
		}

		elem_proxy &operator=(const elem_proxy &other) {
			std::move(*this).value() = other.value();
			return *this;
		}

		const T &value() const& { return val ? *val : vec->get(index); }
		T&  value() &&          { return val ? *val : vec->elems.try_emplace(index).first->second; }

		operator const T&() const& { return value(); }

	private:
		sparse_vector *vec;
		std::size_t index;
		std::optional<T> val;
	};

public:
	const T &get(std::size_t i) const {
		auto it = elems.find(i);
		if(it == elems.end())
			return zero;
		else
			return it->second;
	}

	const T &operator[](std::size_t i) const { return get(i); }

	elem_proxy operator[](std::size_t i) { return elem_proxy{this, i}; }

	std::size_t size() const { return elems.size(); }

private:
	std::map<std::size_t, T> elems;
	T zero{};
};

void foo(double &d) {
	d = 99;
}

int main() {
	sparse_vector<double> v;

	// good
	v[3] = 5.2;
	int x = v[4];
	std::cout << v[3] << std::endl;
	std::cout << x << std::endl;
	std::cout << v.size() << std::endl;
	std::cout << std::endl;

	// good
	auto y = v[3];
	auto z = v[4];
	std::cout << y << std::endl;
	std::cout << z << std::endl;
	y = 111;
	z = 234;
	std::cout << v[3] << std::endl;
	std::cout << y << std::endl;
	std::cout << z << std::endl;
	std::cout << v.size() << std::endl;
	std::cout << std::endl;

	// good
	v[5] = v[3];
	v[6] = v[4];		// okay, not ideal?
	std::cout << v[5] << std::endl;
	std::cout << v[6] << std::endl;
	std::cout << v.size() << std::endl;
	std::cout << std::endl;

	// bad
	auto &&aa = v[3];
	aa = 999;
	std::cout << v[3] << std::endl;		// should modify v[3], but doesn't
	std::cout << aa << std::endl;

	// good
	//auto &bb = v[3];  // error (cannot bind auto& to temporary)
}
