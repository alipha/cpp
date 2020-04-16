#ifndef LIPH_ZIP_ITERATOR_H
#define LIPH_ZIP_ITERATOR_H

#include <cstddef>
#include <iterator>
#include <tuple>


namespace std {
    template<typename... T>
    void swap(std::tuple<T&...> left, std::tuple<T&...> right) {
        std::tuple<T...> temp = left;
        left = right;
        right = temp;
    }
}


namespace detail
{
    template<int... Is>
    struct seq { };

    template<int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

    template<int... Is>
    struct gen_seq<0, Is...> : seq<Is...> { };

    template<typename T, typename F, int... Is>
    void for_each(T&& t, F f, seq<Is...>)
    {
        auto l = { (f(std::get<Is>(t)), 0)... };
        (void)l;
    }
    
    template<typename T, typename F, int... Is>
    auto transform(T&& t, F f, seq<Is...>)
    {
        return std::tie(f(std::get<Is>(t))...);
    }
}


template<typename... Ts, typename F>
void for_each_in_tuple(std::tuple<Ts...> &t, F f)
{
    detail::for_each(t, f, detail::gen_seq<sizeof...(Ts)>());
}

template<typename... Ts, typename F>
auto transform_tuple(std::tuple<Ts...> &t, F f)
{
    return detail::transform(t, f, detail::gen_seq<sizeof...(Ts)>());
}


template<typename... It>
struct zip_iterator
{
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::tuple<typename It::value_type...>;
    using difference_type = std::ptrdiff_t;
    using reference = std::tuple<typename It::reference...>;

    zip_iterator() {}
    zip_iterator(It... its) : its(its...) {}
    
    std::tuple<typename It::reference...> operator*() {
        return transform_tuple(its, [](auto &&it) -> auto& { return *it; });
    }
    
    std::tuple<typename It::reference...> operator[](std::ptrdiff_t n) {
        return transform_tuple(its, [n](auto &&it) -> auto& { return it[n]; });
    }
    
    const std::tuple<typename It::reference...> operator*() const {
        return transform_tuple(its, [](auto &&it) -> auto&& { return *it; });
    }
    
    const std::tuple<typename It::reference...> operator[](std::ptrdiff_t n) const {
        return transform_tuple(its, [n](auto &&it) -> auto&& { return it[n]; });
    }
    
    zip_iterator &operator++() {
        for_each_in_tuple(its, [](auto &&it) { ++it; });
        return *this;
    }
    
    zip_iterator operator++(int) {
        auto ret = *this;
        for_each_in_tuple(its, [](auto &&it) { ++it; });
        return ret;
    }
    
    zip_iterator &operator--() {
        for_each_in_tuple(its, [](auto &&it) { --it; });
        return *this;
    }
    
    zip_iterator operator--(int) {
        auto ret = *this;
        for_each_in_tuple(its, [](auto &&it) { --it; });
        return ret;
    }
    
    zip_iterator& operator+=(std::ptrdiff_t n) {
        for_each_in_tuple(its, [n](auto &&it) { it += n; });
        return *this;
    }
    
    zip_iterator& operator-=(std::ptrdiff_t n) {
        for_each_in_tuple(its, [n](auto &&it) { it -= n; });
        return *this;
    }
    
    zip_iterator operator+(std::ptrdiff_t n) const {
        auto ret = *this;
        return ret += n;
    }
    
    zip_iterator operator-(std::ptrdiff_t n) const {
        auto ret = *this;
        return ret -= n;
    }
    
    friend zip_iterator operator+(std::ptrdiff_t n, const zip_iterator &it) {
        return it + n;
    }
    
    friend std::ptrdiff_t operator-(const zip_iterator &left, const zip_iterator &right) {
        return std::get<0>(left.its) - std::get<0>(right.its);
    }

    /* todo: make these comparison operators more generic */
    bool operator==(const zip_iterator& rhs) const { return std::get<0>(its) == std::get<0>(rhs.its); }
    bool operator!=(const zip_iterator& rhs) const { return std::get<0>(its) != std::get<0>(rhs.its); }
    bool operator<(const zip_iterator& rhs) const { return std::get<0>(its) < std::get<0>(rhs.its); }
    bool operator>(const zip_iterator& rhs) const { return std::get<0>(its) > std::get<0>(rhs.its); }
    bool operator<=(const zip_iterator& rhs) const { return std::get<0>(its) <= std::get<0>(rhs.its); }
    bool operator>=(const zip_iterator& rhs) const { return std::get<0>(its) >= std::get<0>(rhs.its); }

    std::tuple<It...> its;
};


namespace std {
    template<typename... It>
    struct iterator_traits<zip_iterator<It...>> {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::tuple<typename It::value_type...>;
        using difference_type = std::ptrdiff_t;
        using reference = std::tuple<typename It::reference...>;
    };
}

#endif

