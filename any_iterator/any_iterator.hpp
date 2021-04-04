#ifndef LIPH_ANY_ITERATOR_HPP
#define LIPH_ANY_ITERATOR_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>


// TODO: std::hash, any_it_less, any_it_equal_to, constness

namespace liph {
    
    
struct uninitialized_any_iterator : std::logic_error {
    uninitialized_any_iterator() : std::logic_error("performing operations on an uninitialized any_iterator") {}
};


namespace detail {
    

template<typename T> struct in_place_t {};


template<bool UseSbo>
struct sbo_construct {
    template<typename Derived, typename... Args>
    static Derived *construct(char *, in_place_t<Derived>, Args&&... args) {
        return new Derived(std::forward<Args>(args)...);
    }
};

template<>
struct sbo_construct<true> {
    template<typename Derived, typename... Args>
    static Derived *construct(char *buffer, in_place_t<Derived>, Args&&... args) {
        return new (buffer) Derived(std::forward<Args>(args)...);
    }
};



template<typename Base, std::size_t MaxSize = 3 * sizeof(void*)>
struct sbo {
    sbo() : ptr(nullptr) {}

    template<typename Derived, typename... Args>
    sbo(in_place_t<Derived> ip, Args&&... args) { construct(ip, std::forward<Args>(args)...); }

    sbo(const sbo &) = delete;
    sbo(sbo &&) = delete;
    sbo &operator=(const sbo &) = delete;
    sbo &operator=(sbo &&) = delete;

    Base &operator*() const { return *ptr; }
    Base *operator->() const { return ptr; }

    template<typename Derived, typename... Args>
    void replace(in_place_t<Derived> ip, Args&&... args) {
        free();
        ptr = nullptr;
        construct(ip, std::forward<Args>(args)...);
    }

    ~sbo() { free(); }


    alignas(alignof(std::max_align_t)) char buffer[MaxSize];
    Base *ptr;

private:
    template<typename Derived, typename... Args>
    void construct(in_place_t<Derived> ip, Args&&... args) {
        ptr = sbo_construct<sizeof(Derived) <= MaxSize>::construct(buffer, ip, std::forward<Args>(args)...);
    }

    void free() {
       void *vp = ptr;
        if(vp && vp >= buffer && vp < &ptr)
            ptr->~Base();
        else
            delete ptr;
    }
};


template<typename ValueType>
struct post_inc_proxy_base {
    virtual ~post_inc_proxy_base() = default;
    virtual const ValueType &get_value() const = 0;
};

template<typename ValueType, typename Proxy>
struct post_inc_proxy_impl : post_inc_proxy_base<ValueType> {
    post_inc_proxy_impl(Proxy &&proxy) : proxy(std::forward<Proxy>(proxy)) {}

    const ValueType &get_value() const { return *proxy; }

    Proxy proxy;
};


template<typename ValueType>
struct post_inc_proxy {
    template<typename Proxy>
    post_inc_proxy(Proxy &&proxy) : proxy(new post_inc_proxy_impl<ValueType, Proxy>(std::forward<Proxy>(proxy))) {}
    
    post_inc_proxy(post_inc_proxy &&) = default;

    post_inc_proxy(const post_inc_proxy &) = delete;
    post_inc_proxy &operator=(const post_inc_proxy &) = delete;
    post_inc_proxy &operator=(post_inc_proxy &&) = delete;

    ValueType operator*() const { return proxy->get_value(); }

    std::unique_ptr<post_inc_proxy_base<ValueType>> proxy;
};


template<typename ValueType>
struct value_wrapper {
    value_wrapper(ValueType &&value) : value(std::move(value)) {}
    
    ValueType *operator->() { return &value; }

    ValueType value;
};


template<typename ValueType>
struct in_it_holder_base {
    virtual ~in_it_holder_base() = default;
    
    virtual std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const = 0;
    
    virtual bool equal_to(const in_it_holder_base<ValueType> &other) const = 0;
    
    virtual void next() = 0;

    virtual post_inc_proxy<ValueType> post_inc() = 0;
    
    virtual ValueType get_value() const = 0;
};

template<typename ValueType, typename It>
struct in_it_holder : in_it_holder_base<ValueType> {
    // TODO: what about const ValueType? use *std::declval<It> instead? what about const_iterator?
    static_assert(std::is_same<ValueType, typename std::iterator_traits<It>::value_type>::value, "iterator's value_type does not match T in any_input_iterator<T>");
    static_assert(!std::is_same<std::output_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value, "any_input_iterator cannot hold an output iterator");
    
    in_it_holder(It it) : it(std::move(it)) {}
    
    std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const override {
        return std::unique_ptr<in_it_holder_base<ValueType>>(new in_it_holder<ValueType, It>(*this));
    }
    
    bool equal_to(const in_it_holder_base<ValueType> &other) const override { return it == static_cast<const in_it_holder<ValueType, It>&>(other).it; }
    
    void next() override { ++it; }

    post_inc_proxy<ValueType> post_inc() override { return it++; }
    
    ValueType get_value() const override { return *it; }
    
    It it;
};


template<typename ValueType>
struct out_it_holder_base {
    virtual ~out_it_holder_base() = default;
    
    virtual std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const = 0;
    
    virtual void next() = 0;
    
    virtual void set(ValueType value) const = 0;
};

template<typename ValueType, typename It>
struct out_it_holder : out_it_holder_base<ValueType> {
    // TODO: what about const ValueType? use *std::declval<It> instead? what about const_iterator?
    static_assert(std::is_same<ValueType, typename std::iterator_traits<It>::value_type>::value, "iterator's value_type does not match T in any_output_iterator<T>");
    static_assert(!std::is_same<std::input_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value, "any_output_iterator cannot hold an input iterator");
    
    out_it_holder(It it) : it(std::move(it)) {}
    
    std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const override { 
        return std::unique_ptr<out_it_holder_base<ValueType>>(new out_it_holder<ValueType, It>(*this));
    }
    
    void next() override { ++it; }
    
    void set(ValueType value) const override { *it = std::move(value); }
    
    It it;
};


template<typename ValueType>
struct fwd_it_holder_base {
    virtual ~fwd_it_holder_base() = default;
    
    virtual std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const = 0;
    virtual std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const = 0;
    virtual std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const = 0;
    
    virtual bool equal_to(const fwd_it_holder_base<ValueType> &other) const = 0;
    
    virtual void next() = 0;
    
    virtual ValueType &get() const = 0;
};

template<typename ValueType, typename It>
struct fwd_it_holder : fwd_it_holder_base<ValueType> {
    // TODO: what about const ValueType? use *std::declval<It> instead? what about const_iterator?
    static_assert(std::is_same<ValueType, typename std::iterator_traits<It>::value_type>::value, "iterator's value_type does not match T in any_forward_iterator<T>");
    static_assert(std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value, "iterator is not compatible with a forward iterator");
    
    fwd_it_holder(It it) : it(std::move(it)) {}
    
    std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const override { 
        return std::unique_ptr<fwd_it_holder_base<ValueType>>(new fwd_it_holder<ValueType, It>(*this)); 
    }
    
    std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const override { 
        return std::unique_ptr<in_it_holder_base<ValueType>>(new in_it_holder<ValueType, It>(it));
    }

    std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const override { 
        return std::unique_ptr<out_it_holder_base<ValueType>>(new out_it_holder<ValueType, It>(it));  // TODO: check if copy/move constructible and throw an exception if not
    }
    
    bool equal_to(const fwd_it_holder_base<ValueType> &other) const override { return it == static_cast<const fwd_it_holder<ValueType, It>&>(other).it; }
    
    void next() override { ++it; }
    
    ValueType &get() const override { return *it; }
    
    It it;
};


template<typename ValueType>
struct bidir_it_holder_base {
    virtual ~bidir_it_holder_base() = default;
    virtual std::unique_ptr<bidir_it_holder_base<ValueType>> clone_as_bidir() const = 0;
    virtual std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const = 0;
    virtual std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const = 0;
    virtual std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const = 0;
    
    virtual bool equal_to(const bidir_it_holder_base<ValueType> &other) const = 0;
    
    virtual void next() = 0;
    virtual void prev() = 0;
    
    virtual ValueType &get() const = 0;
};

template<typename ValueType, typename It>
struct bidir_it_holder : bidir_it_holder_base<ValueType> {
    // TODO: what about const ValueType? use *std::declval<It> instead? what about const_iterator?
    static_assert(std::is_same<ValueType, typename std::iterator_traits<It>::value_type>::value, "iterator's value_type does not match T in any_bidirectional_iterator<T>");
    static_assert(std::is_base_of<std::bidirectional_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value, "iterator is not compatible with a bidirectional iterator");
    
    bidir_it_holder(It it) : it(std::move(it)) {}
    
    std::unique_ptr<bidir_it_holder_base<ValueType>> clone_as_bidir() const override { 
        return std::unique_ptr<bidir_it_holder_base<ValueType>>(new bidir_it_holder<ValueType, It>(*this));
    }
    
    std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const override { 
        return std::unique_ptr<in_it_holder_base<ValueType>>(new in_it_holder<ValueType, It>(it)); 
    }

    std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const override { 
        return std::unique_ptr<out_it_holder_base<ValueType>>(new out_it_holder<ValueType, It>(it));  // TODO: check if copy/move constructible and throw an exception if not
    }

    std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const override { 
        return std::unique_ptr<fwd_it_holder_base<ValueType>>(new fwd_it_holder<ValueType, It>(it)); 
    }
    
    bool equal_to(const bidir_it_holder_base<ValueType> &other) const override { return it == static_cast<const bidir_it_holder<ValueType, It>&>(other).it; }
    
    void next() override { ++it; }
    void prev() override { --it; }
    
    ValueType &get() const override { return *it; }
    
    It it;
};


template<typename ValueType>
struct rand_it_holder_base {
    virtual ~rand_it_holder_base() = default;
    virtual void clone_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) const = 0;
    virtual std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const = 0;
    virtual std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const = 0;
    virtual std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const = 0;
    virtual std::unique_ptr<bidir_it_holder_base<ValueType>> clone_as_bidir() const = 0;
    
    virtual void move_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) = 0;
    
    virtual bool equal_to(const rand_it_holder_base<ValueType> &other) const = 0;
    virtual bool less(const rand_it_holder_base<ValueType> &other) const = 0;
    
    virtual void next() = 0;
    virtual void prev() = 0;
    
    virtual void add(std::ptrdiff_t n) = 0;
    
    virtual std::ptrdiff_t diff(const rand_it_holder_base<ValueType> &other) const = 0;
    
    virtual ValueType &get() const = 0;
};

template<typename ValueType, typename It>
struct rand_it_holder : rand_it_holder_base<ValueType> {
    // TODO: what about const ValueType? use *std::declval<It> instead? what about const_iterator?
    static_assert(std::is_same<ValueType, typename std::iterator_traits<It>::value_type>::value, "iterator's value_type does not match T in any_random_access_iterator<T>");
    static_assert(std::is_base_of<std::random_access_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value, "iterator is not compatible with a random_access iterator");
    
    rand_it_holder(It it) : it(std::move(it)) {}
    
    void clone_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) const override {
        dest.replace(detail::in_place_t<rand_it_holder<ValueType, It>>{}, *this);
    }
    
    std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const override { 
        return std::unique_ptr<in_it_holder_base<ValueType>>(new in_it_holder<ValueType, It>(it)); 
    }

    std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const override { 
        return std::unique_ptr<out_it_holder_base<ValueType>>(new out_it_holder<ValueType, It>(it));  // TODO: check if copy/move constructible and throw an exception if not
    }

    std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const override { 
        return std::unique_ptr<fwd_it_holder_base<ValueType>>(new fwd_it_holder<ValueType, It>(it)); 
    }

    std::unique_ptr<bidir_it_holder_base<ValueType>> clone_as_bidir() const override { 
        return std::unique_ptr<bidir_it_holder_base<ValueType>>(new bidir_it_holder<ValueType, It>(it)); 
    }
    
    void move_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) override {
        dest.replace(detail::in_place_t<rand_it_holder<ValueType, It>>{}, std::move(*this));
    }
    
    
    bool equal_to(const rand_it_holder_base<ValueType> &other) const override { return it == static_cast<const rand_it_holder<ValueType, It>&>(other).it; }
    bool less(const rand_it_holder_base<ValueType> &other) const override { return it < static_cast<const rand_it_holder<ValueType, It>&>(other).it; }
    
    void next() override { ++it; }
    void prev() override { --it; }
    
    void add(std::ptrdiff_t n) override { it += n; }
    
    std::ptrdiff_t diff(const rand_it_holder_base<ValueType> &other) const override { return it - static_cast<const rand_it_holder<ValueType, It>&>(other).it; }
    
    ValueType &get() const override { return *it; }
    
    It it;
};


template<typename ValueType>
struct empty_it_holder : in_it_holder_base<ValueType>, out_it_holder_base<ValueType>, fwd_it_holder_base<ValueType>, bidir_it_holder_base<ValueType> {
    //void clone_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) const override {
    //    dest.replace(detail::in_place_t<empty_it_holder<ValueType>>{});
    //}

    std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const override { 
        return std::unique_ptr<in_it_holder_base<ValueType>>(new empty_it_holder<ValueType>()); 
    }

    std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const override { 
        return std::unique_ptr<out_it_holder_base<ValueType>>(new empty_it_holder<ValueType>());
    }

    std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const override { 
        return std::unique_ptr<fwd_it_holder_base<ValueType>>(new empty_it_holder<ValueType>());
    }

    std::unique_ptr<bidir_it_holder_base<ValueType>> clone_as_bidir() const override {
        return std::unique_ptr<bidir_it_holder_base<ValueType>>(new empty_it_holder<ValueType>());
    }
    
    //void move_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) override {
    //    dest.replace(detail::in_place_t<empty_it_holder<ValueType>>{});
    //}

    
    bool equal_to(const in_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    bool equal_to(const fwd_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    bool equal_to(const bidir_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    //bool equal_to(const rand_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }

    //bool less(const rand_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    
    void next() override { throw uninitialized_any_iterator(); }
    void prev() override { throw uninitialized_any_iterator(); }
    
    post_inc_proxy<ValueType> post_inc() override { throw uninitialized_any_iterator(); }
    
    //void add(std::ptrdiff_t) override { throw uninitialized_any_iterator(); }
    
    //std::ptrdiff_t diff(const rand_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    
    ValueType get_value() const override { throw uninitialized_any_iterator(); }
    ValueType &get() const override { throw uninitialized_any_iterator(); }
    
    void set(ValueType) const override { throw uninitialized_any_iterator(); }
};


template<typename ValueType>
struct empty_rand_it_holder : rand_it_holder_base<ValueType> {
    void clone_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) const override {
        dest.replace(detail::in_place_t<empty_rand_it_holder<ValueType>>{});
    }

    std::unique_ptr<in_it_holder_base<ValueType>> clone_as_in() const override { 
        return std::unique_ptr<in_it_holder_base<ValueType>>(new empty_it_holder<ValueType>()); 
    }

    std::unique_ptr<out_it_holder_base<ValueType>> clone_as_out() const override { 
        return std::unique_ptr<out_it_holder_base<ValueType>>(new empty_it_holder<ValueType>());
    }

    std::unique_ptr<fwd_it_holder_base<ValueType>> clone_as_fwd() const override { 
        return std::unique_ptr<fwd_it_holder_base<ValueType>>(new empty_it_holder<ValueType>());
    }

    std::unique_ptr<bidir_it_holder_base<ValueType>> clone_as_bidir() const override {
        return std::unique_ptr<bidir_it_holder_base<ValueType>>(new empty_it_holder<ValueType>());
    }
    
    void move_as_rand(detail::sbo<rand_it_holder_base<ValueType>> &dest) override {
        dest.replace(detail::in_place_t<empty_rand_it_holder<ValueType>>{});
    }

    
    bool equal_to(const rand_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }

    bool less(const rand_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    
    void next() override { throw uninitialized_any_iterator(); }
    void prev() override { throw uninitialized_any_iterator(); }
    
    void add(std::ptrdiff_t) override { throw uninitialized_any_iterator(); }
    
    std::ptrdiff_t diff(const rand_it_holder_base<ValueType> &) const override { throw uninitialized_any_iterator(); }
    
    ValueType &get() const override { throw uninitialized_any_iterator(); }
};


} // namespace detail

template<typename ValueType> struct any_input_iterator;
template<typename ValueType> struct any_output_iterator;
template<typename ValueType> struct any_forward_iterator;
template<typename ValueType> struct any_bidirectional_iterator;
template<typename ValueType> struct any_random_access_iterator;


template<typename It>
struct is_any_iterator { static constexpr bool value = false; };

template<typename ValueType>
struct is_any_iterator<any_input_iterator<ValueType>> { static constexpr bool value = true; };

template<typename ValueType>
struct is_any_iterator<any_output_iterator<ValueType>> { static constexpr bool value = true; };

template<typename ValueType>
struct is_any_iterator<any_forward_iterator<ValueType>> { static constexpr bool value = true; };

template<typename ValueType>
struct is_any_iterator<any_bidirectional_iterator<ValueType>> { static constexpr bool value = true; };

template<typename ValueType>
struct is_any_iterator<any_random_access_iterator<ValueType>> { static constexpr bool value = true; };



template<typename ValueType>
struct any_input_iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = ValueType;
    using difference_type = std::ptrdiff_t;
    using pointer = const ValueType*;
    using reference = const ValueType&;

    any_input_iterator() : it(new detail::empty_it_holder<ValueType>) {}
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_input_iterator(It &&it) : it(new detail::in_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(it))) {}
    
    any_input_iterator(const any_input_iterator &other) : it(other.it->clone_as_in()) {}
    any_input_iterator(any_input_iterator &&other) : it(std::move(other.it)) {}
    
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_input_iterator &operator=(It &&new_it) {
        it.reset(new detail::in_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(new_it)));
        return *this;
    }
    
    any_input_iterator &operator=(const any_input_iterator &other) {
        it = other.it->clone_as_in();
        return *this;
    }
    
    any_input_iterator &operator=(any_input_iterator &&other) {
        it = std::move(other.it);
        return *this;
    }
    
    
    any_input_iterator(const any_forward_iterator<ValueType> &other) : it(other.it->clone_as_in()) {}
    
    any_input_iterator &operator=(const any_forward_iterator<ValueType> &other) {
        it = other.it->clone_as_in();
        return *this;
    }
        
    
    any_input_iterator(const any_bidirectional_iterator<ValueType> &other) : it(other.it->clone_as_in()) {}
    
    any_input_iterator &operator=(const any_bidirectional_iterator<ValueType> &other) {
        it = other.it->clone_as_in();
        return *this;
    }
    
    
    any_input_iterator(const any_random_access_iterator<ValueType> &other) : it(other.it->clone_as_in()) {}
    
    any_input_iterator &operator=(const any_random_access_iterator<ValueType> &other) {
        it = other.it->clone_as_in();
        return *this;
    }
    
    ValueType operator*() const { return it->get_value(); }
    detail::value_wrapper<ValueType> operator->() const { return it->get_value(); }

    any_input_iterator &operator++() { it->next(); return *this; }
    
    detail::post_inc_proxy<ValueType> operator++(int) { return it->post_inc(); } 
    
private:
    template<typename T>
    friend bool operator==(const any_input_iterator<T> &left, const any_input_iterator<T> &right);

    std::unique_ptr<detail::in_it_holder_base<ValueType>> it;
};


template<typename ValueType>
bool operator==(const any_input_iterator<ValueType> &left, const any_input_iterator<ValueType> &right) {
    return left.it->equal_to(*right.it);
}

template<typename ValueType>
bool operator!=(const any_input_iterator<ValueType> &left, const any_input_iterator<ValueType> &right) {
    return !(left == right);
}



template<typename ValueType>
struct any_output_iterator {
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = void;

    any_output_iterator() : it(new detail::empty_it_holder<ValueType>) {}
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_output_iterator(It &&it) : it(new detail::in_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(it))) {}
    
    any_output_iterator(const any_output_iterator &other) : it(other.it->clone_as_out()) {}
    any_output_iterator(any_output_iterator &&other) : it(std::move(other.it)) {}
    
    
    template<typename It, typename = typename std::enable_if<
            !is_any_iterator<typename std::decay<It>::type>::value 
            && !std::is_convertible<typename std::decay<It>::type, ValueType>::value 
        >::type>
    any_output_iterator &operator=(It &&new_it) {
        it.reset(new detail::in_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(new_it)));
        return *this;
    }

    any_output_iterator &operator=(const ValueType &value) {
        it->set(value);
        return *this;
    }
    
    any_output_iterator &operator=(ValueType &&value) {
        it->set(std::move(value));
        return *this;
    }
    
    
    any_output_iterator &operator=(const any_output_iterator &other) {
        it = other.it->clone_as_out();
        return *this;
    }
    
    any_output_iterator &operator=(any_output_iterator &&other) {
        it = std::move(other.it);
        return *this;
    }
    
    
    any_output_iterator(const any_forward_iterator<ValueType> &other) : it(other.it->clone_as_out()) {}
    
    any_output_iterator &operator=(const any_forward_iterator<ValueType> &other) {
        it = other.it->clone_as_out();
        return *this;
    }
        
    
    any_output_iterator(const any_bidirectional_iterator<ValueType> &other) : it(other.it->clone_as_out()) {}
    
    any_output_iterator &operator=(const any_bidirectional_iterator<ValueType> &other) {
        it = other.it->clone_as_out();
        return *this;
    }
    
    
    any_output_iterator(const any_random_access_iterator<ValueType> &other) : it(other.it->clone_as_out()) {}
    
    any_output_iterator &operator=(const any_random_access_iterator<ValueType> &other) {
        it = other.it->clone_as_out();
        return *this;
    }
    
    
    any_output_iterator &operator*() { return *this; }
    any_output_iterator &operator++() { return *this; }

private:
    std::unique_ptr<detail::out_it_holder_base<ValueType>> it;
};



template<typename ValueType>
struct any_forward_iterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = ValueType;   
    using difference_type = std::ptrdiff_t;
    using pointer = ValueType*;
    using reference = ValueType&;

    any_forward_iterator() : it(new detail::empty_it_holder<ValueType>) {}
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_forward_iterator(It &&it) : it(new detail::fwd_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(it))) {}
    
    any_forward_iterator(const any_forward_iterator &other) : it(other.it->clone_as_fwd()) {}
    any_forward_iterator(any_forward_iterator &&other) : it(std::move(other.it)) {}
    
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_forward_iterator &operator=(It &&new_it) {
        it.reset(new detail::fwd_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(new_it)));
        return *this;
    }
    
    any_forward_iterator &operator=(const any_forward_iterator &other) {
        it = other.it->clone_as_fwd();
        return *this;
    }
    
    any_forward_iterator &operator=(any_forward_iterator &&other) {
        it = std::move(other.it);
        return *this;
    }
    
    
    any_forward_iterator(const any_bidirectional_iterator<ValueType> &other) : it(other.it->clone_as_fwd()) {}
    
    any_forward_iterator &operator=(const any_bidirectional_iterator<ValueType> &other) {
        it = other.it->clone_as_fwd();
        return *this;
    }
    
    
    any_forward_iterator(const any_random_access_iterator<ValueType> &other) : it(other.it->clone_as_fwd()) {}
    
    any_forward_iterator &operator=(const any_random_access_iterator<ValueType> &other) {
        it = other.it->clone_as_fwd();
        return *this;
    }
    
    
    ValueType &operator*() const { return it->get(); }
    ValueType *operator->() const { return &it->get(); }
    
    
    any_forward_iterator &operator++() { it->next(); return *this; }
    
    any_forward_iterator operator++(int) { 
        any_forward_iterator<ValueType> ret(*this);
        it->next();
        return ret;
    } 
    
private:
    template<typename T>
    friend bool operator==(const any_forward_iterator<T> &left, const any_forward_iterator<T> &right);

    std::unique_ptr<detail::fwd_it_holder_base<ValueType>> it;
};


template<typename ValueType>
bool operator==(const any_forward_iterator<ValueType> &left, const any_forward_iterator<ValueType> &right) {
    return left.it->equal_to(*right.it);
}

template<typename ValueType>
bool operator!=(const any_forward_iterator<ValueType> &left, const any_forward_iterator<ValueType> &right) {
    return !(left == right);
}



template<typename ValueType>
struct any_bidirectional_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = ValueType;   
    using difference_type = std::ptrdiff_t;
    using pointer = ValueType*;
    using reference = ValueType&;

    any_bidirectional_iterator() : it(new detail::empty_it_holder<ValueType>) {}
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_bidirectional_iterator(It &&it) : it(new detail::bidir_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(it))) {}
    
    any_bidirectional_iterator(const any_bidirectional_iterator &other) : it(other.it->clone_as_bidir()) {}
    any_bidirectional_iterator(any_bidirectional_iterator &&other) : it(std::move(other.it)) {}
    
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_bidirectional_iterator &operator=(It &&new_it) {
        it.reset(new detail::bidir_it_holder<ValueType, typename std::decay<It>::type>(std::forward<It>(new_it)));
        return *this;
    }
    
    any_bidirectional_iterator &operator=(const any_bidirectional_iterator &other) {
        it = other.it->clone_as_bidir();
        return *this;
    }
    
    any_bidirectional_iterator &operator=(any_bidirectional_iterator &&other) {
        it = std::move(other.it);
        return *this;
    }
    
    
    any_bidirectional_iterator(const any_random_access_iterator<ValueType> &other) : it(other.it->clone_as_bidir()) {}
    
    any_bidirectional_iterator &operator=(const any_random_access_iterator<ValueType> &other) {
        it = other.it->clone_as_bidir();
        return *this;
    }
    
    
    ValueType &operator*() const { return it->get(); }
    ValueType *operator->() const { return &it->get(); }
    
    
    any_bidirectional_iterator &operator++() { it->next(); return *this; }
    
    any_bidirectional_iterator operator++(int) { 
        any_bidirectional_iterator<ValueType> ret(*this);
        it->next();
        return ret;
    } 
    
    any_bidirectional_iterator &operator--() { it->prev(); return *this; }
    
    any_bidirectional_iterator operator--(int) { 
        any_bidirectional_iterator<ValueType> ret(*this);
        it->prev();
        return ret;
    } 
    
private:
    template<typename T>
    friend bool operator==(const any_bidirectional_iterator<T> &left, const any_bidirectional_iterator<T> &right);

    std::unique_ptr<detail::bidir_it_holder_base<ValueType>> it;
};


template<typename ValueType>
bool operator==(const any_bidirectional_iterator<ValueType> &left, const any_bidirectional_iterator<ValueType> &right) {
    return left.it->equal_to(*right.it);
}

template<typename ValueType>
bool operator!=(const any_bidirectional_iterator<ValueType> &left, const any_bidirectional_iterator<ValueType> &right) {
    return !(left == right);
}



template<typename ValueType>
struct any_random_access_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = ValueType;   
    using difference_type = std::ptrdiff_t;
    using pointer = ValueType*;
    using reference = ValueType&;

    any_random_access_iterator() : it(detail::in_place_t<detail::empty_rand_it_holder<ValueType>>{}) {}
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_random_access_iterator(It &&it) : it(detail::in_place_t<detail::rand_it_holder<ValueType, typename std::decay<It>::type>>{}, std::forward<It>(it)) {}
    
    any_random_access_iterator(const any_random_access_iterator &other) : it() { other.it->clone_as_rand(it); }

    any_random_access_iterator(any_random_access_iterator &&other) : it() { other.it->move_as_rand(it); }
    
    
    template<typename It, typename = typename std::enable_if<!is_any_iterator<typename std::decay<It>::type>::value>::type>
    any_random_access_iterator &operator=(It &&new_it) {
        it.replace(detail::in_place_t<detail::rand_it_holder<ValueType, typename std::decay<It>::type>>{}, std::forward<It>(new_it));
        return *this;
    }
    
    any_random_access_iterator &operator=(const any_random_access_iterator &other) {
        other.it->clone_as_rand(it);
        return *this;
    }
    
    any_random_access_iterator &operator=(any_random_access_iterator &&other) {
        other.it->move_as_rand(it);
        return *this;
    }
    
    
    ValueType &operator*() const { return it->get(); }
    ValueType *operator->() const { return &it->get(); }
    
    
    any_random_access_iterator &operator++() { it->next(); return *this; }
    
    any_random_access_iterator operator++(int) { 
        any_random_access_iterator<ValueType> ret(*this);
        it->next();
        return ret;
    } 
    
    any_random_access_iterator &operator--() { it->prev(); return *this; }
    
    any_random_access_iterator operator--(int) { 
        any_random_access_iterator<ValueType> ret(*this);
        it->prev();
        return ret;
    } 
   
    any_random_access_iterator &operator+=(std::ptrdiff_t n) {
        it->add(n);
        return *this;
    }

    any_random_access_iterator &operator-=(std::ptrdiff_t n) {
        it->add(-n);
        return *this;
    }

private:
    template<typename T>
    friend bool operator==(const any_random_access_iterator<T> &left, const any_random_access_iterator<T> &right);

    template<typename T>
    friend bool operator<(const any_random_access_iterator<T> &left, const any_random_access_iterator<T> &right);

    template<typename T>
    friend bool operator>(const any_random_access_iterator<T> &left, const any_random_access_iterator<T> &right);

    template<typename T>
    friend std::ptrdiff_t operator-(const any_random_access_iterator<T> &left, const any_random_access_iterator<T> &right);


    detail::sbo<detail::rand_it_holder_base<ValueType>> it;
};


template<typename ValueType>
bool operator==(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return left.it->equal_to(*right.it);
}

template<typename ValueType>
bool operator!=(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return !(left == right);
}

template<typename ValueType>
bool operator<(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return left.it->less(*right.it);
}

template<typename ValueType>
bool operator>=(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return !(left < right);
}

template<typename ValueType>
bool operator>(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return right.it->less(*left.it);
}

template<typename ValueType>
bool operator<=(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return !(right < left);
}

template<typename ValueType>
std::ptrdiff_t operator-(const any_random_access_iterator<ValueType> &left, const any_random_access_iterator<ValueType> &right) {
    return left.it->diff(*right.it);
}

template<typename ValueType>
any_random_access_iterator<ValueType> operator+(const any_random_access_iterator<ValueType> &it, std::ptrdiff_t n) {
    any_random_access_iterator<ValueType> ret(it);
    return ret += n;
}

template<typename ValueType>
any_random_access_iterator<ValueType> operator+(std::ptrdiff_t n, const any_random_access_iterator<ValueType> &it) {
    return it + n;
}

template<typename ValueType>
any_random_access_iterator<ValueType> operator-(const any_random_access_iterator<ValueType> &it, std::ptrdiff_t n) {
    return it + -n;
}



} // namespace liph

#endif

