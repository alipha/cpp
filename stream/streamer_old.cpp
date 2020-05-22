#ifndef LIPH_STREAM_STREAM_HPP
#define LIPH_STREAM_STREAM_HPP

#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>


namespace stream {


namespace detail {


/*
template<typename Container, Func>
struct container_stream : stream_gen<Func> {

};
*/

template<typename Src, typename Dest, typename Func = decltype(std::declval<Dest>().init())>
struct stream_pipe {
    stream_pipe(Src s, Dest d) : src(std::move(s)), dest(std::move(d)), func(dest.init()) {}

    auto next() { return func(src); }

    Src src;
    Dest dest;
    Func func;
};

/*
template<typename Container, typename Op>
struct stream_pipe {
    template<typename C>
    stream_pipe(C &&c, Op o) : cont(std::forward<C>(c)), current(cont.begin()), op(std::move(o)) {}

    auto next() {
        if(current == cont.end())
            return std::optional();
        else
            return std::optional(*current++);
    }

    Container cont;
    decltype(cont.begin()) current;
    Op op;
};


template<typename C, typename Op>
stream_pipe(C &&, Op) -> stream_pipe<C&&, Op>;



template<typename T, typename Op>
struct stream_pipe<stream_pipe<T>, Op> {
};
*/

/*
template<>
class stream_operation {
};
*/


template<typename Func>
struct default_gen_init {

};


} // namespace detail



template<typename Func, typename InitFunc>
class stream_gen {
public:
    stream_gen(Func f) : func(std::move(f)) {}
    stream_gen(Func f, InitFunc i) : func(std::move(f)), init_func(std::move(i)) {}

    stream_gen &operator()() { return *this; }

    template<typename... Args>
    auto operator()(Args&&... args) {

        
        std::tuple<Args...> arg_tuple{std::forward<Args>(args)...};

        return stream_gen([func, a = std::move(arg_tuple)](auto &&prev_op) {
            return std::apply([&func, p = std::forward<decltype(prev_op)>(prev_op)](auto&&... arguments) { 
                return func(std::forward<decltype(p)>(p), arguments...);
            }, a);
        };
        */
        //return detail::stream_generation(func, std::forward<Args>(args)...);
    }

    template<typename PrevOp>
    auto run(PrevOp &&prev_op) { return func(std::forward<PrevOp>(prev_op)); }

    auto init() { return init_func(func); }

private:
    Func func;
    InitFunc init_func;
};


template<typename Func>
stream_gen(Func) -> stream_gen<Func, detail::default_gen_init<Func>>;

template<typename Func, typename InitFunc>
stream_gen(Func, InitFunc) -> stream_gen<Func, InitFunc>;



template<typename Func>  // TODO: , typename InitFunc = void>
class stream_op {
public:
    stream_op(Func f) : func(std::move(f)) {}

    stream_op &operator()() { return *this; }

    template<typename... Args>
    auto operator()(Args&&... args) {
        std::tuple<Args...> arg_tuple{std::forward<Args>(args)...};

        return stream_op([func, a = std::move(arg_tuple)](auto &&prev_op) {
            return std::apply([&func, p = std::forward<decltype(prev_op)>(prev_op)](auto&&... arguments) { 
                return func(std::forward<decltype(p)>(p), arguments...);
            }, a);
        };
        //return detail::stream_operation(func, std::forward<Args>(args)...);
    }

    template<typename PrevOp>
    auto run(PrevOp &&prev_op) { return func(std::forward<PrevOp>(prev_op)); }

private:
    Func func;
};



static stream_gen range([](auto &&start, auto &&end) {
    if(start != end)
        return std::optional((*start)++);
    else
        return std::optional();
});

    
static stream_gen container([](auto &&cont, auto &&current) {
        if(current == cont.end())
            return std::optional();
        else
            return std::optional((*current)++);

    },
    [](auto &&func, auto &&cont) {
        decltype(cont.begin()) it = cont.begin();
        return [
            f = std::forward<decltype(func)>(func), 
            &cont, 
            i = std::move(it)
        ]() mutable { return f(cont, i); };
    }
);



template<typename Container, typename Func>
auto operator|(Container &&c, stream_op<Func> op) {
    return detail::stream_pipe(container(std::forward<Container>(c)), std::move(op));
}

template<typename Src, typename Dest, typename Func>
auto operator|(detail::stream_pipe<Src, Dest> p, stream_op<Func> op) {
    return detail::stream_pipe(std::move(p), std::move(op));
}


} // namespace stream

#endif
