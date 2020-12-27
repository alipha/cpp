#ifndef LIPH_STREAM_BASIC_HPP
#define LIPH_STREAM_BASIC_HPP

#include "core.hpp"
#include <stdexcept>
#include <vector>


namespace stream {



class single_error : public streamer_error {
public:
    single_error(const std::string &what) : streamer_error(what) {}
    single_error(const char *what) : streamer_error(what) {}
};


namespace detail {


template<typename T, typename U, typename = std::void_t<>>
constexpr bool has_less_than = false;

template<typename T, typename U>
constexpr bool has_less_than<T, U, std::void_t<decltype(std::declval<T>() < std::declval<U>())>> = true;


struct upto_gen {
    template<typename T, typename U>
    constexpr std::optional<T> operator()(T &start, U &end) const {
        if constexpr(has_less_than<T, U>) {
            if(start < end)
                return start++;
        } else {
            if(start != end)
                return start++;
        }

        return {};
    }

    template<typename T, typename U, typename V>
    constexpr std::optional<T> operator()(T &start, U &end, V &step) const {
        if constexpr(has_less_than<T, U>) {
            if(start < end) {
                T ret = start;
                start += step;
                return ret;
        } else {
            if(start != end) {
                T ret = start;
                start += step;
                return ret;
            }
        }

        return {};
    }
};
struct upto_init {
    template<typename T>
    constexpr auto operator()(T &&end) const {
        return std::make_tuple(T(), std::forward<T>(end));
    }

    template<typename T, typename U>
    constexpr auto operator()(T &&start, U &&end) const {
        return std::make_tuple(std::forward<T>(start), std::forward<U>(end));
    }

    template<typename T, typename U, typename V>
    constexpr auto operator()(T &&start, U &&end, V &&step) const {
        return std::make_tuple(std::forward<T>(start), std::forward<U>(end), std::forward<V>(step));
    }
};


struct downto_gen {
    template<typename T, typename U>
    constexpr std::optional<T> operator()(T &start, U &end) const {
        if constexpr(has_less_than<U, T>) {
            if(end < start)
                return start--;
        } else {
            if(start != end)
                return start--;
        }

        return {};
    }

    template<typename T, typename U, typename V>
    constexpr std::optional<T> operator()(T &start, U &end, V &step) const {
        if constexpr(has_less_than<U, T>) {
            if(end < start) {
                T ret = start;
                start -= step;
                return ret;
        } else {
            if(start != end) {
                T ret = start;
                start -= step;
                return ret;
            }
        }

        return {};
    }
};
struct downto_init {
    template<typename T, typename U>
    constexpr auto operator()(T &&start, U &&end) const {
        return std::make_tuple(std::forward<T>(start), std::forward<U>(end));
    }

    template<typename T, typename U, typename V>
    constexpr auto operator()(T &&start, U &&end, V &&step) const {
        return std::make_tuple(std::forward<T>(start), std::forward<U>(end), std::forward<V>(step));
    }
};


struct mapping_op {
    template<typename Op, typename Func>
    constexpr auto operator()(Op &prev_op, Func &f) const {
        using T = std::decay_t<decltype(f(std::move(*prev_op.next())))>;

        if(auto value = prev_op.next())
            return std::optional<T>(f(std::move(*value)));
        else
            return std::optional<T>();
    }
};


struct flat_mapping_op {
    template<typename Op, typename Func, typename Cont, typename Pipe, typename It>
    constexpr auto operator()(Op &prev_op, Func &f, Cont &cont, Pipe &pipe, It &it) const {
        while(cont || (cont = prev_op.next())) {
            if(!pipe) {
                pipe = f(*cont);
                it = std::begin(*pipe);
            }

            if(*it != std::end(*pipe))
                return *(*it)++;

            pipe.reset();
            cont.reset();
        }

        return std::optional<std::decay_t<decltype(**it)>>();
    }
};
struct flat_mapping_init {
    template<typename Op, typename Func>
    constexpr auto operator()(Op &prev_op, Func &&func) const {
        using Pipe = decltype(func(*prev_op.next()));
        return std::make_tuple(std::forward<Func>(func), typename Op::value_opt_type(), std::optional<Pipe>(), std::optional<typename Pipe::iterator>());
    }
};


struct filter_op {
    template<typename Op, typename Func>
    constexpr auto operator()(Op &prev_op, Func &f) const {
        typename Op::value_opt_type value;
        while((value = prev_op.next()) && !f(*value)) {}
        return value;
    }
};


struct exclude_op {
    template<typename Op, typename Func>
    constexpr auto operator()(Op &prev_op, Func &f) const {
        typename Op::value_opt_type value;
        while((value = prev_op.next()) && f(*value)) {}
        return value;
    }
};


// TODO: move to extra.hpp
struct adj_unique_op {
    template<typename Op>
    constexpr auto operator()(Op &prev_op, typename Op::value_opt_type &prev_value) const {
        typename Op::value_opt_type value;
        while((value = prev_op.next()) && value == prev_value) {}
        prev_value = value;
        return value;
    }
};
struct adj_unique_init {
    template<typename Op>
    constexpr std::tuple<typename Op::value_opt_type> operator()(Op &) const {
        return {};
    }
};


struct as_vector_term {
    template<typename Op>
    constexpr auto operator()(Op &prev_op) const {
        return std::vector<typename Op::value_type>(std::begin(prev_op), std::end(prev_op));
    }
};


template<typename T>
struct as_term {
    template<typename Op>
    constexpr T operator()(Op &prev_op) const {
        return T(std::begin(prev_op), std::end(prev_op));
    }
};


struct first_term {
    template<typename Op>
    constexpr auto operator()(Op &prev_op) const { return prev_op.next(); }
};


struct single_term {
    template<typename Op>
    constexpr auto operator()(Op &prev_op) const { 
        auto ret = prev_op.next(); 
        if(ret && prev_op.next())
            throw single_error("stream contains more than a single value");
        return ret;
    }
};


struct last_term {
    template<typename Op>
    constexpr auto operator()(Op &prev_op) const {
        typename Op::value_type value;
        while(auto next = prev_op.next())
            value = std::move(next);
        return value;
    }
};




} // namespace detail


constexpr inline stream_gen upto{detail::upto_gen{}, detail::upto_init{}};
constexpr inline stream_gen downto{detail::downto_gen{}, detail::downto_init{}};
constexpr inline stream_op mapping{detail::mapping_op{}};
constexpr inline stream_op flat_mapping{detail::flat_mapping_op{}, detail::flat_mapping_init{}};
constexpr inline stream_op filter{detail::filter_op{}};
constexpr inline stream_op exclude{detail::exclude_op{}};
constexpr inline stream_op adj_unique{detail::adj_unique_op{}, detail::adj_unique_init{}};  // TODO: move to extra.hpp
constexpr inline stream_term as_vector{detail::as_vector_term{}};

template<typename T>
constexpr stream_term as{detail::as_term<T>{}};

constexpr inline stream_term first{detail::first_term{}};   // unbounded
constexpr inline stream_term single{detail::single_term{}};  // unbounded
constexpr inline stream_term last{detail::last_term{}};

} // namespace stream

#endif
