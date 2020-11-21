#ifndef LIPH_STREAM_STREAM_HPP
#define LIPH_STREAM_STREAM_HPP

#include "core.hpp"
#include "lambda.hpp"


namespace stream {


namespace detail {


template<typename T>
struct stream_iterator {
    stream_iterator() : src(nullptr), value() {}
    stream_iterator(std::function<std::optional<T>()> *s) : src(&s), value((*src)()) {}

    stream_iterator &operator++() { value = (*src)(); return *this; }

    stream_iterator operator++(int) {
        iterator it = *this;
        value = (*src)();
        return it;
    }

    T &operator*() { return *value; }

    std::function<std::optional<T>()> *src;    
    std::optional<T> value;
};


template<typename T>
bool operator==(const stream_iterator<T> &left, const stream_iterator<T> &right) {
    return left.value.has_value() == right.value.has_value();
}

template<typename T>
bool operator!=(const stream_iterator<T> &left, const stream_iterator<T> &right) {
    return !(left == right);
}


} // namespace detail



inline stream_gen make_stream(
	overload
	& [](auto &first, auto &last) {
			if(first == last)
				return std::optional<std::decay_t<decltype(*first)>>();
			else
				return std::optional(*first++);
		}
    & detail::container_gen(),

	overload
	& detail::container_init()
    & [](auto &cont) {
			return std::make_tuple(std::begin(cont), std::end(cont));
		}
	& [](auto &&first, auto &&last) {
			return std::make_tuple(std::forward<decltype(first)>(first), std::forward<decltype(last)>(last));
		}
	& [](auto *p, std::size_t len) {
			return std::make_tuple(p, p + len);
		}
);



template<typename T>
class streamer {
public:
    template<typename U, typename V>
    streamer(U &&u, V &&v) : src([s = make_stream(std::forward<U>(u), std::forward<V>(v))]() mutable { return s.next(); }) {}

    template<typename Container>
    streamer(Container &&c)
        : src([s = detail::gen(container(std::forward<Container>(c)))]() mutable { return s.next(); }) {}

    template<typename Src, typename Dest, typename Params>
    streamer(detail::pipe<Src, Dest, Params> p) 
        : src([s = std::move(p)]() mutable { return s.next(); }) {}

    template<typename... Args>
    streamer(stream_gen<Args...> g)
        : src([s = detail::gen(std::move(g))]() mutable { return s.next(); }) {}

    template<typename U>
    streamer(streamer<U> &&other)
        : src([s = std::move(other.s)]() mutable { return s(); }) {}


    streamer(const streamer &) = delete;
    streamer &operator=(const streamer &) = delete;

    streamer(streamer &&) = default;
    streamer &operator=(streamer &&) = default;


    using iterator = detail::stream_iterator<T>;
    using value_opt_type = std::optional<T>;
    using value_type = T;
    
    std::optional<T> next() { return src(); }

    iterator begin() { return iterator(&src); }
    iterator end() { return iterator(); }

private:
    std::function<std::optional<T>()> src;
};


template<typename U, typename V>
streamer(U&& u, V&&) -> streamer<std::remove_reference_t<decltype(*u)>>;

template<typename Container>
streamer(Container &&c) -> streamer<std::remove_reference_t<decltype(*std::begin(std::forward<Container>(c)))>>;

template<typename Src, typename Dest, typename Params>
streamer(detail::pipe<Src, Dest, Params>) -> streamer<typename detail::pipe<Src, Dest, Params>::value_type>; 

template<typename... Args>
streamer(stream_gen<Args...> g) -> streamer<std::remove_reference_t<decltype(*g.begin())>>;



template<typename T, typename... Args>
auto operator|(streamer<T> &&s, stream_op<Args...> op) {
    return detail::pipe(std::move(s), std::move(op));
}

template<typename T, typename... Args>
auto operator|(streamer<T> &&s, stream_term<Args...> op) {
    return detail::pipe(std::move(s), std::move(op)).next();
}


} // namespace stream



namespace std {
    template<typename T>
    struct iterator_traits<stream::detail::stream_iterator<T>> {
        using difference_type = std::size_t;
        using value_type = T;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = input_iterator_tag;
    };
}

#endif
