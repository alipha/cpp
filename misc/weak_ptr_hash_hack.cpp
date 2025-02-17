#include <memory>
#include <unordered_set>
#include <iostream>

namespace std {
	template<typename T>
	struct hash<std::weak_ptr<T>> {
		static_assert(sizeof(std::weak_ptr<T>) == sizeof(std::shared_ptr<T>));

		std::size_t operator()(const std::weak_ptr<T> &wp) const {
			auto &sp = reinterpret_cast<const std::shared_ptr<T>&>(wp);
			return std::hash<std::shared_ptr<T>>{}(sp);
		}
	};

	template<typename T, typename U>
	bool operator==(const std::weak_ptr<T> &lhs, const std::weak_ptr<U> &rhs) {
		auto &l = reinterpret_cast<const std::shared_ptr<T>&>(lhs);
		auto &r = reinterpret_cast<const std::shared_ptr<T>&>(rhs);
		return l == r;
	}

	template<typename T>
	struct equal_to<std::weak_ptr<T>> {
		constexpr bool operator()( const std::weak_ptr<T>& lhs, const std::weak_ptr<T>& rhs ) const {
			return lhs == rhs;
		}
	};
}

int main() {
	int *p = new int(3);
	std::shared_ptr<int> sp(p);
	std::weak_ptr<int> wp = sp;

	int *p2 = new int(6);
	std::shared_ptr<int> sp2(p2);
	std::weak_ptr<int> wp2 = sp2;

	int *p3 = new int(9);
	std::shared_ptr<int> sp3(p3);
	std::weak_ptr<int> wp3 = sp3;
	std::unordered_set<std::weak_ptr<int>> s;

	s.insert(wp);
	s.insert(wp2);
	s.insert(wp);
	std::cout << s.size() << std::endl;
	std::cout << s.contains(wp) << std::endl;
	std::cout << s.contains(wp2) << std::endl;
	std::cout << s.contains(wp3) << std::endl;

	sp.reset();
	sp2.reset();

	std::cout << s.size() << std::endl;
	std::cout << s.contains(wp) << std::endl;
	std::cout << s.contains(wp2) << std::endl;
	std::cout << s.contains(wp3) << std::endl;
}
