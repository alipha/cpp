#ifndef LIPH_STREAM_BASIC_HPP
#define LIPH_STREAM_BASIC_HPP

#include "stream.hpp"
#include <vector>


namespace stream {


static stream_gen range([](auto &start, auto &end) {
    if(start != end)
        return std::optional(start++);
    else
        return std::optional<std::decay_t<decltype(start)>>();
});

  
static stream_op mapping([](auto &prev_op, auto &f) {
    if(auto value = prev_op.next())
        return std::optional(f(*value));
    else
        return decltype(prev_op.next())();
});


stream_op filter([](auto &prev_op, auto &f) {
    decltype(prev_op.next()) value;
    while((value = prev_op.next()) && !f(*value)) {}
    return value;
});



static stream_op adj_unique([](auto &prev_op, auto &prev_value) {
        decltype(prev_op.next()) value;
        while((value = prev_op.next()) && value == prev_value) {}
        prev_value = value;
        return value;
    },
    [](auto &prev_op) {
        return std::tuple<typename std::remove_reference_t<decltype(prev_op)>::result_opt_type>();
    }
);


static stream_term as_vector([](auto &prev_op) {
    std::vector<std::remove_reference_t<decltype(*prev_op.next())>> result;
    while(auto value = prev_op.next())
        result.push_back(*value);
    return result;
});

/*
template<typename T>
stream_op as([](auto &prev_op) {
    return T(prev_op.begin(), prev_op.end());
});
*/

} // namespace stream

#endif
