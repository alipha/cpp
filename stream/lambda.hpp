#ifndef LIPH_STREAM_LAMBDA_HPP
#define LIPH_STREAM_LAMBDA_HPP

#include <type_traits>
#include <utility>


namespace stream {


namespace detail {


template<typename Func, typename Next>
struct lambda_overload : Func, Next {

    lambda_overload(Func f, Next n) : Func(std::move(f)), Next(std::move(n)) {}

    using Func::operator();
    using Next::operator();
};


template<>
struct lambda_overload<void, void> {
    void operator()(struct invalid_arguments_to_lambda&) {}
};

	
template<typename Func, typename Next, typename NewFunc>
auto operator&(lambda_overload<Func, Next> prev, NewFunc &&new_func) {
    return lambda_overload(std::forward<NewFunc>(new_func), std::move(prev));
}


} // namespace detail


inline detail::lambda_overload<void, void> overload;


} // namespace stream

#endif
