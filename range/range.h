#ifndef LIPH_RANGE_H
#define LIPH_RANGE_H

#include <functional>
#include <iterator>
#include <utility>


struct increment {
    template<typename T>
    void operator()(T &&x) const { ++x; }
};

struct decrement {
    template<typename T>
    void operator()(T &&x) const { --x; }
};


template<typename T, typename IncFunc, typename EqualFunc>
struct count_iterator {
    count_iterator(T value, IncFunc func) : value(value), func(func) {}

    count_iterator operator++() {
        func(value);
        return *this;
    }
    
    count_iterator operator++(int) {
        auto ret = *this;
        func(value);
        return ret;
    }
    
    const T &operator*() const { return value; }
    T &operator*() { return value; }
    
    T value;
    IncFunc func;
};


namespace std {
    template<typename T, typename IncFunc, typename EqualFunc>
    struct iterator_traits<count_iterator<T, IncFunc, EqualFunc>> {
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using reference = T&;
    };
}


template<typename T, typename IncFunc, typename EqualFunc>
bool operator==(const count_iterator<T, IncFunc, EqualFunc> &left, const count_iterator<T, IncFunc, EqualFunc> &right) {
    return EqualFunc()(*left, *right);
}

template<typename T, typename IncFunc, typename EqualFunc>
bool operator!=(const count_iterator<T, IncFunc, EqualFunc> &left, const count_iterator<T, IncFunc, EqualFunc> &right) {
    return !EqualFunc()(*left, *right);
}


template<typename T, typename IncFunc = increment, typename EqualFunc = std::equal_to<T>>
struct range {
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_iterator = count_iterator<T, IncFunc, EqualFunc>;
    using iterator = count_iterator<T, IncFunc, EqualFunc>;
    
    range(T first, T last, IncFunc func = IncFunc()) 
        : first(std::move(first)), last(std::move(last)), func(std::move(func)) {}
    
    iterator begin() const { return iterator(first, func); }
    iterator cbegin() const { return iterator(first, func); }
    
    iterator end() const { return iterator(last, func); }
    iterator cend() const { return iterator(last, func); }
    
private:
    T first;
    T last;
    IncFunc func;
};

#endif
