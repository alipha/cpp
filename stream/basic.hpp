#ifndef LIPH_STREAM_BASIC_HPP
#define LIPH_STREAM_BASIC_HPP

#include "core.hpp"
#include <vector>


namespace stream {


namespace detail {


struct range_gen {
    template<typename T, typename U>
    constexpr std::optional<T> operator()(T &start, U &end) const {
        if(start != end)
            return start++;
        else
            return {};
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


struct filter_op {
    template<typename Op, typename Func>
    constexpr auto operator()(Op &prev_op, Func &f) const {
        typename Op::value_opt_type value;
        while((value = prev_op.next()) && !f(*value)) {}
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
        std::vector<typename Op::value_type> result;
        while(auto value = prev_op.next())
            result.push_back(std::move(*value));
        return result;
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




} // namespace detail


constexpr inline stream_gen range{detail::range_gen{}};
constexpr inline stream_op mapping{detail::mapping_op{}};
constexpr inline stream_op filter{detail::filter_op{}};
constexpr inline stream_op adj_unique{detail::adj_unique_op{}, detail::adj_unique_init{}};  // TODO: move to extra.hpp
constexpr inline stream_term as_vector{detail::as_vector_term{}};

template<typename T>
constexpr stream_term as{detail::as_term<T>{}};

constexpr inline stream_term first{detail::first_term{}};

} // namespace stream

#endif
