#ifndef ZIP_ITERATOR_HPP
#define ZIP_ITERATOR_HPP

#include <cstddef>
#include <functional>
#include <iterator>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>


namespace detail {


template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


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



template<typename Func, std::size_t Index, typename... Tuples>
auto combine_tuples(std::index_sequence<Index>, Func &&func, Tuples&&... tuples) {
    return std::make_tuple(func(std::get<Index>(std::forward<Tuples>(tuples))...));
}


template<typename Func, std::size_t FirstIndex, std::size_t SecondIndex, std::size_t... RestIndexes, typename... Tuples>
auto combine_tuples(std::index_sequence<FirstIndex, SecondIndex, RestIndexes...>, Func &&func, Tuples&&... tuples) {
    return std::tuple_cat(combine_tuples(std::index_sequence<FirstIndex>(), std::forward<Func>(func), std::forward<Tuples>(tuples)...),
        combine_tuples(std::index_sequence<SecondIndex, RestIndexes...>(), std::forward<Func>(func), std::forward<Tuples>(tuples)...));
}


template<typename Func, typename FirstTuple, typename... Tuples>
auto combine_tuples(Func &&func, FirstTuple &&first_tuple, Tuples&&... tuples) {
    using First = std::decay_t<FirstTuple>;

    constexpr std::size_t size = std::tuple_size_v<First>;
    auto eq = std::equal_to<std::size_t>();
    static_assert((eq(size, std::tuple_size_v<std::decay_t<Tuples>>) && ...), "All tuples must be the same size.");
    
    return detail::combine_tuples(std::make_index_sequence<size>(), std::forward<Func>(func), 
        std::forward<FirstTuple>(first_tuple), std::forward<Tuples>(tuples)...);
}


template<typename Func, typename... Tuples>
bool all_true(Func &&func, Tuples&&... tuples) {
    return std::apply([](auto... values) { return (values && ...); },
        combine_tuples(func, tuples...)
    );
}


} // namespace detail



template<typename... Its>
struct zip_iterator;


template<typename... Types>
struct zip_value {
private:
    using variant_type = std::variant<std::monostate, std::tuple<Types...>, std::tuple<Types&...>>;
    
public:
    zip_value() : values() {}

    template<typename... OtherTypes>
    explicit zip_value(const std::tuple<OtherTypes...> &other) : values(std::tuple<Types...>(other)) {}

    template<typename... OtherTypes>
    explicit zip_value(std::tuple<OtherTypes...> &&other) : values(std::tuple<Types...>(std::move(other))) {}


    zip_value(const zip_value &other) : values(init(other.values)) {}

    zip_value(zip_value &&other) : values(init(std::move(other.values))) {}
    

    zip_value &operator=(const zip_value &other) { return assign(other.values); }
    
    zip_value &operator=(zip_value &&other) { return assign(std::move(other.values)); }

    template<typename... OtherTypes>
    zip_value &operator=(const std::tuple<OtherTypes...> &other) { return assign_tuple(other); }

    template<typename... OtherTypes>
    zip_value &operator=(std::tuple<OtherTypes...> &&other) { return assign_tuple(std::move(other)); }
    
    zip_value &reset(const zip_value &other) {
        values = other.values;
        return *this;
    }
    
    zip_value &reset(zip_value &&other) {
        values = std::move(other.values);
        return *this;
    }

    operator std::tuple<Types...>() const {
        std::visit(detail::overloaded {
            [](std::monostate) { 
                //if constexpr(std::is_default_constructible_v<std::tuple<Types...>>) {
                //    return {}; 
                //} else {
                    throw std::logic_error("zip_value is empty.");
                //}
            },
            [](auto &v) { return v; }
        }, values);
    }


    template<std::size_t Index>
    auto &get() & { 
        return std::visit(detail::overloaded {
            [](std::monostate) -> std::tuple_element_t<Index, std::tuple<Types...>>& { throw std::logic_error("zip_value is empty."); },
            [](auto &t) -> std::tuple_element_t<Index, std::tuple<Types...>>& { return std::get<Index>(t); }
        }, values);
    }

    template<std::size_t Index>
    auto &&get() && { 
        return std::visit(detail::overloaded {
            [](std::monostate) -> std::tuple_element_t<Index, std::tuple<Types...>>&& { throw std::logic_error("zip_value is empty."); },
            [](auto &&t) -> std::tuple_element_t<Index, std::tuple<Types...>>&& { return std::move(std::get<Index>(t)); }
        }, std::move(values));
    }
    
    template<std::size_t Index>
    const auto &get() const & { 
        return std::visit(detail::overloaded {
            [](std::monostate) -> const std::tuple_element_t<Index, std::tuple<Types...>>& { throw std::logic_error("zip_value is empty."); },
            [](auto &t) -> const std::tuple_element_t<Index, std::tuple<Types...>>& { return std::get<Index>(t); }
        }, values);
    }

private:
    template<typename... Its>
    friend struct zip_iterator;

    template<typename... Ts>
    friend bool operator<(const zip_value<Ts...> &left, const zip_value<Ts...> &right);
    
    
    template<typename... Its>
    zip_value &reset(const std::tuple<Types&...> &other) {
        if constexpr((std::is_const_v<Types> || ...)) {
            using T = decltype(values);
            values.~T();
            new (&values) T(other);
        } else {
            values = std::monostate();
            values = variant_type(other);
        }
        return *this;
    }


    template<typename Other>
    static variant_type init(Other &&other) {
        return std::visit(detail::overloaded {
            [](std::monostate) { return variant_type(); },
            [](auto &&o) { return variant_type(std::tuple<Types...>(std::forward<decltype(o)>(o))); }
        }, std::forward<Other>(other));
    }

    template<typename Other>
    zip_value &assign(Other &&other) {
        std::visit(detail::overloaded {
            [](std::monostate, std::monostate)   {},
            [this](auto &, std::monostate)       { this->values = std::monostate(); },
            [this](std::monostate, auto &&right) { this->values = std::tuple<Types...>(std::forward<decltype(right)>(right)); },
            [](auto &left, auto &&right)         { left = std::forward<decltype(right)>(right); }
        }, values, std::forward<Other>(other));

        return *this;
    }

    template<typename Other>
    zip_value &assign_tuple(Other &&other) {
        std::visit(detail::overloaded {
            [this, &other](std::monostate) { this->values = std::tuple<Types...>(std::forward<decltype(other)>(other)); },
            [&other](auto &left)           { left = std::forward<decltype(other)>(other); }
        }, values);

        return *this;
    }

    variant_type values;
};


template<std::size_t Index, typename... Types>
auto &get(zip_value<Types...> &value) {
    return value.template get<Index>();
}


template<std::size_t Index, typename... Types>
auto &&get(zip_value<Types...> &&value) {
    return std::move(value).template get<Index>();
}


template<std::size_t Index, typename... Types>
const auto &get(const zip_value<Types...> &value) {
    return value.template get<Index>();
}


template<typename... Types>
bool operator<(const zip_value<Types...> &left, const zip_value<Types...> &right) {
    return std::visit(detail::overloaded {
        [](std::monostate, std::monostate) { return false; },
        [](std::monostate, auto &) { return true; },
        [](auto &, std::monostate) { return false; },
        [](auto &l, auto &r) { return l < r; }
    }, left.values, right.values);
}

template<typename... Types>
bool operator>(const zip_value<Types...> &left, const zip_value<Types...> &right) { return right > left; }

template<typename... Types>
bool operator<=(const zip_value<Types...> &left, const zip_value<Types...> &right) { return !(right > left); }

template<typename... Types>
bool operator>=(const zip_value<Types...> &left, const zip_value<Types...> &right) { return !(left < right); }

template<typename... Types>
bool operator!=(const zip_value<Types...> &left, const zip_value<Types...> &right) { return left < right || right < left; }

template<typename... Types>
bool operator==(const zip_value<Types...> &left, const zip_value<Types...> &right) { return !(left != right); }



template<typename... Its>
struct zip_iterator {
    static_assert(sizeof...(Its) > 0, "At least one iterator type needs to be provided.");
    static_assert((!std::is_reference_v<Its> && ...), "zip_iterator cannot hold references to iterators.");
    static_assert((sizeof(typename std::iterator_traits<Its>::iterator_category) && ...), "Not all template arguments are iterator types.");
    
    using iterator_category = decltype(detail::min_iterator_category<Its...>());
    using value_type = zip_value<detail::default_if_void<std::remove_reference_t<typename std::iterator_traits<Its>::reference>, Its>...>;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&; 
    using pointer = value_type*;
    
    zip_iterator() {}
    zip_iterator(std::tuple<Its...> its) : its(std::move(its)) {}
    zip_iterator(Its... its) : its(std::move(its)...) {}
    
    zip_iterator(const zip_iterator &other) = default;
    zip_iterator(zip_iterator &&other) = default;
    
    zip_iterator &operator=(const zip_iterator &other) {
        its = other.its;
        values.reset(other.values);
        return *this;
    }
    
    zip_iterator &operator=(zip_iterator &&other) {
        its = std::move(other.its);
        values.reset(std::move(other.values));
        return *this;
    }
    

    std::tuple<Its...> &get() noexcept { return its; }

    const std::tuple<Its...> &get() const noexcept { return its; }


    reference operator*() const {
        return values.reset(std::apply([](auto&... its) { return std::tie(*its...); }, its));
    }
    
    pointer operator->() const {
        return &values.reset(std::apply([](auto&... its) { return std::tie(*its...); }, its));
    }

    template<typename Cat = iterator_category, typename = std::enable_if_t<std::is_same_v<std::random_access_iterator_tag, Cat>>>
    reference operator[](std::ptrdiff_t n) const {
        return values.reset(std::apply([n](auto&... its) { return std::tie(its[n]...); }, its));
    }


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
    mutable value_type values;
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
        detail::combine_tuples([](auto &left_it, auto &right_it) {
            return left_it - right_it;
        }, left.get(), right.get())
    );
}


template<typename... Its>
bool operator==(const zip_iterator<Its...> &left, const zip_iterator<Its...> &right) {
    return std::apply([](auto... values) { return (values || ...); },
        detail::combine_tuples([](auto &left_it, auto &right_it) {
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


namespace std {
    template<typename... Types>
    struct tuple_size<zip_value<Types...>> : integral_constant<size_t, sizeof...(Types)> {};

    template<size_t Index, typename... Types>
    struct tuple_element<Index, zip_value<Types...>> {
        using type = tuple_element_t<Index, tuple<Types...>>;
    };
}

#endif
