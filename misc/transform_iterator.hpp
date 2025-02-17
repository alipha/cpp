
// updates: https://godbolt.org/z/x1MEca4jj

#include <vector>
#include <iostream>

#include <iterator>
#include <type_traits>
#include <utility>


template<typename It, typename = void>
struct get_difference_type {
	using type = void;
};

template<typename It>
struct get_difference_type<It, decltype(std::declval<typename std::iterator_traits<It>::difference_type>(), (void)0)> {
	using type = typename std::iterator_traits<It>::difference_type;
};


template<typename Transform, bool = false>
class transform_iterator_base {
public:
	transform_iterator_base() {}
	transform_iterator_base(Transform &&transform) : tran(std::move(transform)) {}

	Transform &transform() { return tran; }
	const Transform &transform() const { return tran; }

private:
	Transform tran;
};


template<>
class transform_iterator_base<void, false> {};


template<typename Transform>
class transform_iterator_base<Transform, std::is_class<Transform>::value && !std::is_final<Transform>::value> 
	: private Transform {
public:
	transform_iterator_base() {}
	transform_iterator_base(Transform &&transform) : Transform(std::move(transform)) {}

	Transform &transform() { return *this; }
	const Transform &transform() const { return *this; }
};

// common to all iterator categories. the below classes use `void` category to refer to this class
template<typename It, typename Transform, typename = typename std::iterator_traits<It>::iterator_category>
class transform_iterator : public transform_iterator_base<Transform> {
public:
	using iterator_category = typename std::iterator_traits<It>::iterator_category;
	using difference_type = typename std::iterator_traits<It>::difference_type;

	transform_iterator() {}
	transform_iterator(It it) : it(std::move(it)) {}
	transform_iterator(It it, Transform transform) 
		: transform_iterator_base<Transform>(std::move(transform)), it(std::move(it)) {}

	const It &base() const { return it; }
	It &base() { return it; }

	transform_iterator &operator++() { ++it; return *this; }

private:
	It it;
};


template<typename It, typename Transform>
class transform_iterator<It, Transform, std::input_iterator_tag> 
	: public transform_iterator<It, Transform, void> {
private:
	using base_type = transform_iterator<It, Transform, void>;

public:
	using reference = decltype(std::declval<Transform>()(*std::declval<It>()));
	using value_type = typename std::decay_t<reference>;
	using pointer = std::conditional<std::is_class<value_type>::value, std::remove_reference_t<value_type>*, void>;
	
	using transform_iterator<It, Transform, void>::transform_iterator;
	using base_type::operator++;

	transform_iterator& operator=(It other) { this->base() = std::move(other); return *this; }

	decltype(auto) operator*() const { return this->transform()(*this->base()); }
	decltype(auto) operator*() { return this->transform()(*this->base()); }

	decltype(auto) operator->() const { return &this->transform()(*this->base()); }
	decltype(auto) operator->() { return &this->transform()(*this->base()); }

	transform_iterator operator++(int) { return ++transform_iterator(*this); }
};


template<typename It, typename Transform>
class transform_iterator<It, Transform, std::output_iterator_tag> 
	: public transform_iterator<It, Transform, void> {
private:
	using base_type = transform_iterator<It, Transform, void>;

public:
	using value_type = void;
	using reference = void;
	using pointer = void;

	using base_type::transform_iterator;
	using base_type::operator++;

	transform_iterator& operator=(It other) { this->base() = std::move(other); return *this; }

	template<typename T, typename = std::enable_if_t<!std::is_convertible<std::remove_reference_t<T>, It>::value>>
	decltype(auto) operator=(T &&other) { 
		return *this->base() = this->transform()(std::forward<T>(other)); 
	}

	template<typename T, typename = std::enable_if_t<!std::is_convertible<std::remove_reference_t<T>, It>::value>>
	decltype(auto) operator=(T &&other) const { 
		return *this->base() = this->transform()(std::forward<T>(other)); 
	}

	const transform_iterator& operator*() const { return *this; }
	transform_iterator& operator*() { return *this; }

	transform_iterator& operator++(int) { this->base()++; return *this; }
};


template<typename It, typename Transform>
class transform_iterator<It, Transform, std::forward_iterator_tag> 
	: public transform_iterator<It, Transform, std::input_iterator_tag> {
public:
	using transform_iterator<It, Transform, std::input_iterator_tag>::transform_iterator;

	transform_iterator& operator=(It other) { this->base() = std::move(other); return *this; }
};


template<typename It, typename Transform>
class transform_iterator<It, Transform, std::bidirectional_iterator_tag> 
	: public transform_iterator<It, Transform, std::forward_iterator_tag> {
public:
	using transform_iterator<It, Transform, std::forward_iterator_tag>::transform_iterator;

	transform_iterator& operator=(It other) { this->base() = std::move(other); return *this; }

	transform_iterator &operator--() { --this->base(); return *this;}
	transform_iterator operator--(int) { return --transform_iterator(*this); }
};


template<typename It, typename Transform>
class transform_iterator<It, Transform, std::random_access_iterator_tag> 
	: public transform_iterator<It, Transform, std::bidirectional_iterator_tag> {
public:
	using transform_iterator<It, Transform, std::bidirectional_iterator_tag>::transform_iterator;

	transform_iterator& operator=(It other) { this->base() = std::move(other); return *this; }

	template<typename T>
	decltype(auto) operator[](T &&n) const { return this->transform()(this->base()[std::forward<T>(n)]); }
};


template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
bool operator==(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() == right.base();
}

template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
bool operator<(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() < right.base();
}

template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
bool operator<=(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() <= right.base();
}

template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
bool operator!=(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() != right.base();
}

template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
bool operator>(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() > right.base();
}

template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
bool operator>=(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() >= right.base();
}

template<typename It1, typename Tran1, typename Tag1, typename It2, typename Tran2, typename Tag2>
auto operator-(const transform_iterator<It1, Tran1, Tag1> &left, const transform_iterator<It2, Tran2, Tag2> &right) {
	return left.base() - right.base();
}

template<typename It, typename Tran, typename Tag, typename T>
auto operator+=(transform_iterator<It, Tran, Tag> &it, T &&n) {
	return it.base() += std::forward<T>(n);
}

template<typename It, typename Tran, typename Tag, typename T>
auto operator-=(transform_iterator<It, Tran, Tag> &it, T &&n) {
	return it.base() -= std::forward<T>(n);
}

template<typename It, typename Tran, typename Tag, typename T>
auto operator+(const transform_iterator<It, Tran, Tag> &it, T &&n) {
	return it.base() + std::forward<T>(n);
}

template<typename It, typename Tran, typename Tag, typename T>
auto operator+(T &&n, const transform_iterator<It, Tran, Tag> &it) {
	return std::forward<T>(n) + it.base();
}


template<typename It, typename Transform>
class const_iterator_range {
public:
	
};

template<typename It, typename Transform>
class iterator_range {
public:

};


template<typename Transform, typename It>
auto make_transform_iterator(It it, Transform transform) {
	return transform_iterator<It, Transform>(std::move(it), std::move(transform));
}

template<typename Transform, typename It>
auto make_transform_iterator(It it) {
	return transform_iterator<It, Transform>(std::move(it), {});
}

template<typename Transform, typename It>
auto make_output_transform_iterator(It it, Transform transform) {
	return transform_iterator<It, Transform, std::output_iterator_tag>(std::move(it), std::move(transform));
}

template<typename Transform, typename It>
auto make_output_transform_iterator(It it) {
	return transform_iterator<It, Transform, std::output_iterator_tag>(std::move(it), {});
}

template<typename Transform, typename It>
auto make_end_transform_iterator(It it) { return transform_iterator<It, void>(std::move(it)); }

template<typename Transform, typename Container>
auto make_transform_iterator_range(Container &&cont, Transform transform) {
	return std::make_pair(
		make_transform_iterator(std::begin(cont), transform),
		make_transform_iterator(std::end(cont), transform)
	);
}

template<typename Transform, typename Container>
auto make_transform_iterator_range(Container &&cont) {
	return make_transform_iterator_range<Transform>(cont, {});
}

#if __cplusplus >= 201703L

template<auto Func>
struct functor {
	template<typename... Args>
	auto operator()(Args&&... args) const { return Func(std::forward<Args>(args)...); }
};

#endif


int main() {
	std::vector<int> src{5, 20, 9, 13};
	std::vector<double> dest(src.size());

	auto [first, last] = make_transform_iterator_range(src, [](int x) { return x * 3; });
	std::copy(first, last, dest.begin());

	std::copy(src.begin(), src.end(), make_output_transform_iterator(dest.begin(), [](int x) { return x / 2.0; }));

	for(double d : dest)
		std::cout << d << ' ';
}
