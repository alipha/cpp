#ifndef LIPH_STREAM_STREAM_HPP
#define LIPH_STREAM_STREAM_HPP

#include <iterator>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>


namespace stream {


namespace detail {


template<typename Func, typename Tuple, typename... Args>
auto apply_first(Func &&func, Tuple &&tup, Args&&... args) {
    return std::apply(std::forward<Func>(func), 
            std::tuple_cat(std::forward<Tuple>(tup), std::tuple<Args&&...>(std::forward<Args>(args)...)));
}


template<typename Func, typename Tuple, typename... Args>
auto apply_last(Func &&func, Tuple &&tup, Args&&... args) {
    return std::apply(std::forward<Func>(func), 
            std::tuple_cat(std::tuple<Args&&...>(std::forward<Args>(args)...), std::forward<Tuple>(tup)));
}


template<typename Op, typename... Params>
auto call_stream_gen_run(Op &&op, std::tuple<Params...> &params) {
    using Class = std::remove_reference_t<Op>;
    using Ret = decltype(op.run(std::declval<Params>()...));
    return apply_last(static_cast<Ret(Class::*)(Params&&...)>(&Class::template run<Params...>), params, op);
}


template<typename Op, typename Src, typename... Params>
auto call_stream_op_run(Op &&op, Src &src, std::tuple<Params...> &params) {
    using Class = std::remove_reference_t<Op>;
    using Ret = decltype(op.run(src, std::declval<Params>()...));
    return apply_last(static_cast<Ret(Class::*)(Src&, Params&&...)>(&Class::template run<Src&, Params...>), params, op, src);
}



template<typename Src, typename Params = decltype(std::declval<Src>().init())>
struct gen {
    gen(Src s) : src(std::move(s)), params(src.init()) {}

    auto next() { return call_stream_gen_run(src, params); }

    Src src;
    Params params;
    
    using result_opt_type = decltype(std::declval<gen<Src>>().next());
    using result_type = typename result_opt_type::value_type;
};


template<typename Src, typename Dest, typename Params = decltype(std::declval<Dest>().template init<Src>())>
struct pipe {
    pipe(Src s, Dest d) : src(std::move(s)), dest(std::move(d)), params(dest.template init<Src>()) {}

    auto next() { return call_stream_op_run(dest, src, params); }

    Src src;
    Dest dest;
    Params params;
    
    using result_opt_type = decltype(std::declval<pipe>().next());
    using result_type = typename result_opt_type::value_type;
};


struct default_init {
    template<typename... Args>
    auto operator()(Args&&... args) const { return std::make_tuple(std::forward<Args>(args)...); }
};


struct default_op_init {
    template<typename SrcType, typename... Args>
    auto operator()(SrcType, Args&&... args) const { return std::make_tuple(std::forward<Args>(args)...); }
};


} // namespace detail



template<typename T> struct src_type { using type = T; };


template<typename Func, typename InitFunc = detail::default_init>
class stream_gen {
public:
    stream_gen(Func f) : func(std::move(f)) {}
    stream_gen(Func f, InitFunc i) : func(std::move(f)), init_func(std::move(i)) {}

    stream_gen &operator()() { return *this; }

    template<typename... Args>
    auto operator()(Args&&... args) {   
        auto init = [i = init_func, a = std::tuple<Args&&...>(std::forward<Args>(args)...)](auto&&... more_args) {
            return detail::apply_first(std::move(i), std::move(a), std::forward<decltype(more_args)>(more_args)...);
        };
        return stream_gen<Func, decltype(init)>(func, std::move(init));
     }

    template<typename... Args>
    auto run(Args&&... args) { return func(std::forward<Args>(args)...); }

    auto init() { return init_func(); }

private:
    Func func;
    InitFunc init_func;
};



template<typename Func, typename InitFunc = detail::default_op_init>
class stream_op {
public:
    stream_op(Func f) : func(std::move(f)) {}
    stream_op(Func f, InitFunc i) : func(std::move(f)), init_func(std::move(i)) {}

    stream_op &operator()() { return *this; }

    template<typename... Args>
    auto operator()(Args&&... args) {   
        auto init = [i = init_func, a = std::tuple<Args&&...>(std::forward<Args>(args)...)](auto type, auto&&... more_args) {
            return apply_first(std::move(i), std::tuple_cat(std::make_tuple(type), std::move(a)), std::forward<decltype(more_args)>(more_args)...);
        };
        return stream_op<Func, decltype(init)>(func, std::move(init));
     }

    template<typename... Args>
    auto run(Args&&... args) { return func(std::forward<Args>(args)...); }

    template<typename Src>
    auto init() { return init_func(src_type<Src>()); }

private:
    Func func;
    InitFunc init_func;
};



  
static stream_gen container([](auto &&cont, auto &&current) {
        using std::end;
        if(current == end(cont))
            return std::optional<std::decay_t<decltype(*current)>>();
        else
            return std::optional((*current)++);

    },
    [](auto &&cont) { 
        using std::begin;
        return std::tuple<decltype(cont), decltype(begin(cont))>(
            std::forward<decltype(cont)>(cont), begin(cont)); 
    }       
);



template<typename Container, typename Func, typename InitFunc>
auto operator|(Container &&c, stream_op<Func, InitFunc> op) {
    return detail::pipe(detail::gen(container(std::forward<Container>(c))), std::move(op));
}

template<typename Func, typename InitFunc, typename Func2, typename InitFunc2>
auto operator|(stream_gen<Func, InitFunc> g, stream_op<Func2, InitFunc2> op) {
    return detail::pipe(detail::gen(std::move(g)), std::move(op));
}

template<typename Src, typename Dest, typename Params, typename Func, typename InitFunc>
auto operator|(detail::pipe<Src, Dest, Params> p, stream_op<Func, InitFunc> op) {
    return detail::pipe(std::move(p), std::move(op));
}


} // namespace stream

#endif
