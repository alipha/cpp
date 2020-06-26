#ifndef LIPH_STREAM_CORE_HPP
#define LIPH_STREAM_CORE_HPP

#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>


namespace stream {


namespace detail {


template<typename Func, typename Tuple, typename = std::void_t<>>
constexpr bool can_apply_tuple = false;

template<typename Func, typename... Args>
constexpr bool can_apply_tuple<Func, std::tuple<Args...>, 
          std::void_t<decltype(std::declval<Func>()(std::declval<Args&>()...))>> = true;


template <typename... Args>
auto make_ref_tuple_from_args(Args&... args) { return std::tuple<Args&...>(args...); }


template <size_t... indices, typename T1>
auto make_ref_tuple(T1&& s, std::index_sequence<indices...>)
{
    return make_ref_tuple_from_args(std::get<indices>(std::forward<T1>(s))...);
}

template <typename T1>
auto make_ref_tuple(T1&& s)
{
    constexpr std::size_t N = std::tuple_size<std::remove_reference_t<T1>>::value;
    return make_ref_tuple(std::forward<T1>(s), std::make_index_sequence<N>());
}



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
auto call_stream_gen_run(Op &op, std::tuple<Params&...> &&params) {
    using Class = std::remove_reference_t<Op>;
    using Ret = decltype(op.run(std::declval<Params&>()...));

    return apply_last(static_cast<Ret(Class::*)(Params&...)>(&Class::template run<Params&...>), 
            std::move(params),
            op);
}


template<typename Op, typename Src, typename... Params>
auto call_stream_op_run(Op &op, Src &src, std::tuple<Params&...> params) {
    using Class = std::remove_reference_t<Op>;
    using Ret = decltype(op.run(src, std::declval<Params&>()...));

    return apply_last(static_cast<Ret(Class::*)(Src&, Params&...)>(&Class::template run<Src&, Params&...>),
            std::move(params), 
            op, 
            src);
}



template<typename Pipe, typename Gen = char, typename ValueType = decltype(std::declval<Pipe>().next())>
struct iterator {
    iterator() : gen(), pipe(nullptr), value() {}
    explicit iterator(Pipe *p) : gen(), pipe(p), value(pipe->next()) {}
    explicit iterator(Gen &&g) : gen(std::move(g)), pipe(&*gen), value(pipe->next()) {}

    iterator &operator++() { value = pipe->next(); return *this; }

    iterator operator++(int) {
        iterator it = *this;
        value = pipe->next();
        return it;
    }

    typename ValueType::value_type &operator*() { return *value; }

    std::optional<Gen> gen;
    Pipe *pipe;
    ValueType value;
};


template<typename Pipe, typename Gen, typename ValueType>
bool operator==(const iterator<Pipe, Gen, ValueType> &left, const iterator<Pipe, Gen, ValueType> &right) {
    return left.value.has_value() == right.value.has_value();
}

template<typename Pipe, typename Gen, typename ValueType>
bool operator!=(const iterator<Pipe, Gen, ValueType> &left, const iterator<Pipe, Gen, ValueType> &right) {
    return !(left == right);
}



template<typename Src, typename Params = decltype(std::declval<Src>().init())>
struct gen {
    explicit gen(Src s) : src(std::move(s)), params(src.init()) {}

    auto next() { return call_stream_gen_run(src, make_ref_tuple(params)); }

    Src src;
    Params params;

    using iterator = detail::iterator<gen, gen>;
    using value_opt_type = decltype(std::declval<gen<Src>>().next());
    using value_type = typename value_opt_type::value_type;
    
    iterator begin() { return iterator(this); }
    iterator end() { return iterator(); }
};


template<typename Src, typename Dest, typename Params = decltype(std::declval<Dest>().template init(std::declval<Src&>()))>
struct pipe {
    pipe(Src s, Dest d) : src(std::move(s)), dest(std::move(d)), params(dest.template init(src)) {}
/*
    pipe(const pipe &) = delete;
    pipe &operator=(const pipe &) = delete;

    pipe(pipe &&) = default;
    pipe &operator=(pipe &&) = default;
*/
    auto next() { return call_stream_op_run(dest, src, make_ref_tuple(params)); }

    Src src;
    Dest dest;
    Params params;
    
    using iterator = detail::iterator<pipe>;
    using value_opt_type = decltype(std::declval<pipe>().next());
    using value_type = typename value_opt_type::value_type;
    
    iterator begin() { return iterator(this); }
    iterator end() { return iterator(); }
};



template<typename Func, typename InitFunc>
class stream_gen_base {
public:
    stream_gen_base(Func &&f) : func(std::move(f)) {}
    stream_gen_base(Func &&f, InitFunc &&i) : func(std::move(f)), init_func(std::move(i)) {}

    template<typename... Args>
    auto run(Args&... args) { return func(args...); }

    auto init() { return init_func(); }

protected:
    Func func;
    InitFunc init_func;
};



template<typename Gen, typename InitFunc, typename Func, typename = std::void_t<>>
struct iterator_type { using type = struct error_stream_gen_missing_parameters; };

template<typename Gen, typename InitFunc, typename Func>
struct iterator_type<Gen, InitFunc, Func, 
        std::enable_if_t<can_apply_tuple<Func, decltype(std::declval<InitFunc>()())>>> {
    using type = iterator<gen<Gen>, gen<Gen>>; 
};



struct default_init {
    template<typename... Args>
    auto operator()(Args&&... args) const { return std::make_tuple(std::forward<Args>(args)...); }
};


struct default_op_init {
    template<typename Src, typename... Args>
    auto operator()(Src&, Args&&... args) const { return std::make_tuple(std::forward<Args>(args)...); }
};


} // namespace detail



template<typename Func, typename InitFunc = detail::default_init>
class stream_gen : public detail::stream_gen_base<Func, InitFunc> {
public:
    stream_gen(Func f) : detail::stream_gen_base<Func, InitFunc>(std::move(f)) {}
    stream_gen(Func f, InitFunc i) : detail::stream_gen_base<Func, InitFunc>(std::move(f), std::move(i)) {}

    stream_gen &operator()() { return *this; }

    template<typename... Args>
    auto operator()(Args&&... args) {   
        auto init = [i = this->init_func, a = std::tuple<Args...>(std::forward<Args>(args)...)](auto&&... more_args) {
            return detail::apply_first(std::move(i), std::move(a), std::forward<decltype(more_args)>(more_args)...);
        };
        return stream_gen<Func, decltype(init)>(this->func, std::move(init));
     }

    using iterator = typename detail::iterator_type<
                                detail::stream_gen_base<Func, InitFunc>, 
                                InitFunc,
                                Func
                            >::type;

    auto begin() { return iterator(detail::gen<detail::stream_gen_base<Func, InitFunc>>(*this)); }
    auto end() { return iterator(); }
};



template<typename Func, typename InitFunc = detail::default_op_init>
class stream_op {
public:
    stream_op(Func f) : func(std::move(f)) {}
    stream_op(Func f, InitFunc i) : func(std::move(f)), init_func(std::move(i)) {}

    stream_op &operator()() { return *this; }

    template<typename... Args>
    auto operator()(Args&&... args) {   
        auto init = [i = init_func, a = std::tuple<Args...>(std::forward<Args>(args)...)](auto &prev_op, auto&&... more_args) {
            return apply_first(std::move(i), std::tuple_cat(std::tuple<decltype(prev_op)&>(prev_op), std::move(a)), std::forward<decltype(more_args)>(more_args)...);
        };
        return stream_op<Func, decltype(init)>(func, std::move(init));
     }

    template<typename... Args>
    auto run(Args&... args) { return func(args...); }

    template<typename Src>
    auto init(Src &src) { return init_func(src); }

private:
    Func func;
    InitFunc init_func;
};



template<typename Func, typename InitFunc = detail::default_op_init>
class stream_term : public stream_op<Func, InitFunc> {
public:
    stream_term(Func f) : stream_op<Func, InitFunc>(std::move(f)) {}
    stream_term(Func f, InitFunc i) : stream_op<Func, InitFunc>(std::move(f), std::move(i)) {} 
};



template<typename... Args>
auto smart_make_tuple(Args&&... args) { return std::tuple<Args...>(std::forward<Args>(args)...); }



namespace detail {

struct container_op {
    template<typename Container, typename It>
    auto operator()(Container &cont, std::optional<It> &current) const {
        using std::begin;
        using std::end;
        if(!current)
            current = begin(cont);
        if(*current == end(cont))
            return std::optional<std::decay_t<decltype(**current)>>();
        else
            return std::optional(*(*current)++);

    }
};

struct container_init {
    template<typename Container>
    auto operator()(Container &&cont) const { 
        using std::begin;
        return smart_make_tuple(
            std::forward<Container>(cont), 
            std::optional<decltype(begin(std::forward<Container>(cont)))>()); 
    }
};

} // namespace detail


inline stream_gen container{detail::container_op(), detail::container_init()};



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


template<typename Container, typename Func, typename InitFunc>
auto operator|(Container &&c, stream_term<Func, InitFunc> op) {
    return detail::pipe(detail::gen(container(std::forward<Container>(c))), std::move(op)).next();
}

template<typename Func, typename InitFunc, typename Func2, typename InitFunc2>
auto operator|(stream_gen<Func, InitFunc> g, stream_term<Func2, InitFunc2> op) {
    return detail::pipe(detail::gen(std::move(g)), std::move(op)).next();
}

template<typename Src, typename Dest, typename Params, typename Func, typename InitFunc>
auto operator|(detail::pipe<Src, Dest, Params> p, stream_term<Func, InitFunc> op) {
    return detail::pipe(std::move(p), std::move(op)).next();
}


} // namespace stream


namespace std {
    template<typename Pipe, typename Gen, typename ValueType>
    struct iterator_traits<stream::detail::iterator<Pipe, Gen, ValueType>> {
        using difference_type = std::size_t;
        using value_type = typename ValueType::value_type;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = input_iterator_tag;
    };
}

#endif
