#ifndef LIPH_ENUMERATE_HPP
#define LIPH_ENUMERATE_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>


namespace detail {
    using std::begin;
        
    template<typename Cont>
    struct enumerate_value_type {
        using cont_iterator = std::remove_reference_t<decltype(begin(std::declval<Cont&>()))>;
        std::size_t index;
        cont_iterator it;
    };
    
    template<typename Cont>
    struct enumerate_iterator {
        using value_type = enumerate_value_type<Cont>;
        using cont_iterator = typename value_type::cont_iterator;
                
        enumerate_iterator(std::size_t index, const cont_iterator &it) : value{index, it} {}
        
        value_type &operator*() { return value; }
        
        enumerate_iterator &operator++() {
            ++value.index;
            ++value.it;
            return *this;
        }
        
        value_type value;
    };
    
    template<int Index, typename Cont>
    auto get(enumerate_value_type<Cont> &&it) {
        if      constexpr(Index == 0) return it.index;
        else if constexpr(Index == 1) return *it.it;
    }
    
    template<int Index, typename Cont>
    auto &get(enumerate_value_type<Cont> &it) {
        if      constexpr(Index == 0) return it.index;
        else if constexpr(Index == 1) return *it.it;
    }
    
    template<int Index, typename Cont>
    const auto &get(const enumerate_value_type<Cont> &it) {
        if      constexpr(Index == 0) return it.index;
        else if constexpr(Index == 1) return *it.it;
    }
    
    template<typename Cont>
    bool operator!=(const enumerate_iterator<Cont> &left, const enumerate_iterator<Cont> &right) {
        return left.value.it != right.value.it;
    }
}

namespace std {
    template <typename Cont> 
    struct tuple_size<detail::enumerate_value_type<Cont>> : integral_constant<size_t, 2> { };
    
    template <typename Cont>
    struct tuple_element<0, detail::enumerate_value_type<Cont>> { using type = size_t; };
    
    template <typename Cont> 
    struct tuple_element<1, detail::enumerate_value_type<Cont>> { 
        using type = std::remove_reference_t<decltype(*begin(declval<Cont&>()))>;
    };
}


template<typename Cont>
struct enumerate {
    using iterator = detail::enumerate_iterator<Cont>;
    
    enumerate(Cont &cont) : cont(cont) {}
    
    iterator begin() { 
        using std::begin;
        return {0, begin(cont)};
    }
    
    iterator end() { 
        using std::end;
        using std::size;
        return {size(cont), end(cont)};
    }
    
private:
    Cont &cont;
};


#endif
