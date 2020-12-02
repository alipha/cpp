#ifndef ZIP_ITERATOR_HPP
#define ZIP_ITERATOR_HPP

#include <cstddef>
#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>


namespace std {
    template<typename... T>
    void swap(std::tuple<T&...> left, std::tuple<T&...> right) {
        std::tuple<T...> temp = left;
        left = right;
        right = temp;
    }
}


namespace detail {
	

template<typename Func, std::size_t Index, typename... Tuples>
auto combine_tuples(std::index_sequence<Index>, Func &&func, Tuples&&... tuples) {
    return std::make_tuple(func(std::get<Index>(std::forward<Tuples>(tuples))...));
}


template<typename Func, std::size_t FirstIndex, std::size_t SecondIndex, std::size_t... RestIndexes, typename... Tuples>
auto combine_tuples(std::index_sequence<FirstIndex, SecondIndex, RestIndexes...>, Func &&func, Tuples&&... tuples) {
    return std::tuple_cat(combine_tuples(std::index_sequence<FirstIndex>(), std::forward<Func>(func), std::forward<Tuples>(tuples)...),
        combine_tuples(std::index_sequence<SecondIndex, RestIndexes...>(), std::forward<Func>(func), std::forward<Tuples>(tuples)...));
}


template<typename Cat>
constexpr int iterator_category_order() { 
	if      constexpr(std::is_same_v<std::input_iterator_tag,         Cat>) return 1;
	else if constexpr(std::is_same_v<std::output_iterator_tag,        Cat>) return 2;
	else if constexpr(std::is_same_v<std::forward_iterator_tag,       Cat>) return 3;
	else if constexpr(std::is_same_v<std::bidirectional_iterator_tag, Cat>) return 4;
	else if constexpr(std::is_same_v<std::random_access_iterator_tag, Cat>) return 5;
	else 
		static_assert(sizeof(Cat) && false, "Unknown iteratory category tag."); 
}


template<int Order>
constexpr auto order_to_iterator_category() {
	if      constexpr(Order == 1) return std::input_iterator_tag();
	else if constexpr(Order == 2) return std::output_iterator_tag();
	else if constexpr(Order == 3) return std::forward_iterator_tag();
	else if constexpr(Order == 4) return std::bidirectional_iterator_tag();
	else if constexpr(Order == 5) return std::random_access_iterator_tag();
	else 
		static_assert(Order && false, "Invalid iterator_category_order value.");
}


template<typename It>
constexpr auto min_iterator_category() { return typename std::iterator_traits<It>::iterator_category(); }


template<typename FirstIt, typename SecondIt, typename... RestIts>
constexpr auto min_iterator_category() {
	constexpr int first_order = iterator_category_order<typename std::iterator_traits<FirstIt>::iterator_category>();
	constexpr int rest_order = iterator_category_order<decltype(min_iterator_category<SecondIt, RestIts...>())>();
	
	static_assert(first_order + rest_order != 3, "Cannot have both an input iterator and an output iterator in the same zip_iterator.");
	
	constexpr int min_order = first_order < rest_order ? first_order : rest_order;
	return order_to_iterator_category<min_order>();
}


template<typename T, typename Default>
using default_if_void = std::conditional_t<std::is_same_v<void, T>, Default, T>;
	
	
template<typename Cat>
using enable_if_bidirectional = std::enable_if_t<std::is_same_v<std::bidirectional_iterator_tag, Cat> || std::is_same_v<std::random_access_iterator_tag, Cat>>;


} // namespace detail


template<typename Func, typename FirstTuple, typename... Tuples>
auto combine_tuples(Func &&func, FirstTuple &&first_tuple, Tuples&&... tuples) {
    using First = std::decay_t<FirstTuple>;

    constexpr std::size_t size = std::tuple_size_v<First>;
    auto eq = std::equal_to<std::size_t>();
    static_assert((eq(size, std::tuple_size_v<std::decay_t<Tuples>>) && ...), "All tuples must be the same size.");
    
    return detail::combine_tuples(std::make_index_sequence<size>(), std::forward<Func>(func), 
        std::forward<FirstTuple>(first_tuple), std::forward<Tuples>(tuples)...);
}


namespace detail {

    template<typename Func, typename... Tuples>
    bool all_true(Func &&func, Tuples&&... tuples) {
        return std::apply([](auto... values) { return (values && ...); },
            combine_tuples(func, tuples...)
        );
    }
} // namespace detail



template<typename... Its>
struct zip_iterator {
	static_assert(sizeof...(Its) > 0, "At least one iterator type needs to be provided.");
    static_assert((!std::is_reference_v<Its> && ...), "zip_iterator cannot hold references to iterators.");
	static_assert((sizeof(typename std::iterator_traits<Its>::iterator_category) && ...), "Not all template arguments are iterator types.");
	
	using iterator_category = decltype(detail::min_iterator_category<Its...>());
	using value_type = std::tuple<typename std::iterator_traits<Its>::value_type...>;
	using difference_type = std::ptrdiff_t;
	using reference = std::tuple<detail::default_if_void<typename std::iterator_traits<Its>::reference, Its&>...>;
	//using const_reference = std::tuple<typename std::iterator_traits<const Its>::reference...>;
	using pointer = void;
	
	zip_iterator() {}
    zip_iterator(std::tuple<Its...> its) : its(std::move(its)) {}
	zip_iterator(Its... its) : its(std::move(its)...) {}
	

    std::tuple<Its...> &get() noexcept { return its; }

    const std::tuple<Its...> &get() const noexcept { return its; }


	reference operator*() { return std::apply([](auto&... its) { return std::tie(*its...); }, its); }
	
    /*
	template<typename Cat = iterator_category, typename = std::enable_if_t<!std::is_same_v<std::output_iterator_tag, Cat>>>
	const const_reference operator*() const { 
		return std::apply([](const auto&... its) { return std::tie(*its...); }, its);
	}
    */
	
	
	template<typename Cat = iterator_category, typename = std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, Cat>>>
    reference operator[](std::ptrdiff_t n) {
		std::apply([n](auto&... its) { return std::tie(its[n]...); }, its);
	}
	
    /*
	template<typename Cat = iterator_category, typename = std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, Cat>>>
	const const_reference operator[](std::ptrdiff_t n) const {
		std::apply([n](const auto&... its) { return std::tie(its[n]...); }, its);
	}
	*/

	
	zip_iterator &operator++() { std::apply([](auto&... its) { (++its, ...); }, its); return *this; }
	
	zip_iterator operator++(int) { zip_iterator ret(*this); ++ret; return ret; }
	
	
	template<typename Cat = iterator_category, typename = detail::enable_if_bidirectional<Cat>>
	zip_iterator &operator--() { std::apply([](auto&... its) { (--its, ...); }, its); return *this; }
	
	template<typename Cat = iterator_category, typename = detail::enable_if_bidirectional<Cat>>
	zip_iterator operator--(int) { zip_iterator ret(*this); --ret; return ret; }
	
	
	template<typename Cat = iterator_category, typename = std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, Cat>>>
	zip_iterator &operator+=(std::ptrdiff_t n) {
		std::apply([n](auto&... its) { ((its += n), ...); }, its);
		return *this;
	}
	
	template<typename Cat = iterator_category, typename = std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, Cat>>>
	zip_iterator &operator-=(std::ptrdiff_t n) {
		*this += -n;
		return *this;
	}
	
private:
	std::tuple<Its...> its;
};



template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, zip_iterator<Its...>>
operator+(const zip_iterator<Its...> &it, std::ptrdiff_t n) {
    auto ret = it;
    return ret += n;
}

template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, zip_iterator<Its...>>
operator+(std::ptrdiff_t n, const zip_iterator<Its...> &it) {
    return it + n;
}


template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, zip_iterator<Its...>>
operator-(const zip_iterator<Its...> &it, std::ptrdiff_t n) {
    return it + -n;
}

template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, std::ptrdiff_t>
operator-(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return std::apply([](auto... values) { return std::min(values...); },
        combine_tuples([](auto &left_it, auto &right_it) {
            return left_it - right_it;
        }, left.get(), right.get())
    );
}


template<typename... Its>
bool operator==(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return std::apply([](auto... values) { return (values || ...); },
        combine_tuples([](auto &left_it, auto &right_it) {
            return left_it == right_it;
        }, left.get(), right.get())
    );
}

template<typename... Its>
bool operator!=(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) { return !(left == right); }


template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, bool>
operator<(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return detail::all_true([](auto &left_it, auto &right_it) { return left_it < right_it; },
            left.get(), right.get());
}

template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, bool>
operator<=(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return detail::all_true([](auto &left_it, auto &right_it) { return left_it <= right_it; }, 
            left.get(), right.get());
}

template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, bool>
operator>(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return detail::all_true([](auto &left_it, auto &right_it) { return left_it > right_it; }, 
            left.get(), right.get());
}

template<typename... Its>
std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, typename zip_iterator<Its...>::iterator_category>, bool>
operator>=(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return detail::all_true([](auto &left_it, auto &right_it) { return left_it >= right_it; }, 
            left.get(), right.get());
}



template<typename... Containers>
struct zip_range {
	static_assert(sizeof...(Containers) > 0, "At least one container needs to be provided.");

    using iterator = zip_iterator<std::remove_reference_t<decltype(std::begin(std::declval<Containers&>()))>...>;
    using const_iterator = zip_iterator<std::remove_reference_t<decltype(std::cbegin(std::declval<Containers&>()))>...>;

    using value_type = typename iterator::value_type;
    using reference = typename iterator::reference;
    using difference_type = typename iterator::difference_type;
    using size_type = std::size_t;


    zip_range(Containers&... conts) : containers(conts...) {}


    const_iterator cbegin() const {
        return const_iterator(std::apply([](auto&... conts) { return std::make_tuple(std::cbegin(conts)...); }, containers));
    }

    const_iterator begin() const { return cbegin(); }

    iterator begin() {
        return iterator(std::apply([](auto&... conts) { return std::make_tuple(std::begin(conts)...); }, containers));
    }


    const_iterator cend() const {
        return const_iterator(std::apply([](auto&... conts) { return std::make_tuple(std::cend(conts)...); }, containers));
    }

    const_iterator end() const { return cend(); }

    iterator end() {
        return iterator(std::apply([](auto&... conts) { return std::make_tuple(std::end(conts)...); }, containers));
    }

private:
    std::tuple<Containers&...> containers;
};


template<typename... Containers>
struct const_zip_range : zip_range<const Containers...> {
    const_zip_range(Containers&... conts) : zip_range<const Containers...>(conts...) {}
};


#endif
