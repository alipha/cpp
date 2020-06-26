#ifndef LIPH_STREAM_BASIC_HPP
#define LIPH_STREAM_BASIC_HPP

#include "core.hpp"
#include <vector>


namespace stream {


inline stream_gen range([](auto &start, auto &end) {
    if(start != end)
        return std::optional(start++);
    else
        return std::optional<std::decay_t<decltype(start)>>();
});

  
inline stream_op mapping([](auto &prev_op, auto &f) {
    if(auto value = prev_op.next())
        return std::optional(f(*value));
    else
        return decltype(prev_op.next())();
});


inline stream_op filter([](auto &prev_op, auto &f) {
    decltype(prev_op.next()) value;
    while((value = prev_op.next()) && !f(*value)) {}
    return value;
});



inline stream_op adj_unique([](auto &prev_op, auto &prev_value) {
        decltype(prev_op.next()) value;
        while((value = prev_op.next()) && value == prev_value) {}
        prev_value = value;
        return value;
    },
    [](auto &prev_op) {
        return std::tuple<decltype(prev_op.next())>();
    }
);


inline stream_term as_vector([](auto &prev_op) {
    std::vector<std::remove_reference_t<decltype(*prev_op.next())>> result;
    while(auto value = prev_op.next())
        result.push_back(*value);
    return result;
});


template<typename T>
inline stream_term as([](auto &prev_op) {
    using std::begin;
    using std::end;
    return T(begin(prev_op), end(prev_op));
});


} // namespace stream

#endif
