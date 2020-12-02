#ifndef LIPH_PRECOMPUTE_H
#define LIPH_PRECOMPUTE_H

#include <utility>
#include <array>


template<typename T, std::size_t Max, std::size_t... Rest>
struct precompute {
    template<typename Func>
    constexpr precompute(Func func) : results(call<Func>(func, std::make_index_sequence<Max>())) {}
    
    template<typename... Indexes>
    constexpr T operator()(std::size_t index, Indexes... rest) const { return results[index](rest...); }
    
    std::array<precompute<T, Rest...>, Max> results;
    
private:
    template<std::size_t Index, typename Func>
    static constexpr auto curry(Func func) { 
        return [func](auto... args) constexpr { return func(Index, args...); };
    }
    
    template<typename Func, std::size_t... Indexes>
    static constexpr std::array<precompute<T, Rest...>, Max> call(Func func, std::index_sequence<Indexes...>) {
        return std::array<precompute<T, Rest...>, Max>{curry<Indexes>(func)...};
    }
};


template<typename T, std::size_t Max>
struct precompute<T, Max> {
    template<typename Func>
    constexpr precompute(Func func) : results(call<Func>(func, std::make_index_sequence<Max>())) {}
    
    constexpr T operator()(std::size_t index) const { return results[index]; }
    
    std::array<T, Max> results;

private:
    template<typename Func, std::size_t... Indexes>
    static constexpr std::array<T, Max> call(Func func, std::index_sequence<Indexes...>) {
        return std::array<T, Max>{func(Indexes)...};
    }
};

#endif

