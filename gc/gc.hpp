#ifndef LIPH_GC_HPP
#define LIPH_GC_HPP

#include "gc_detail.hpp"
#include <cstddef>
#include <functional>
#include <new>
#include <utility>
#include <type_traits>


// TODO: const correctness?
// TODO: use allocators?
// TODO: separate node from T
// TODO: exception safe
// TODO: support T[]
// TODO: gc::anchor
// TODO: weak_ptr
// TODO: on gc::ptr constructor, call transverse and check
//       that all gc::ptrs that have been created are
//       reached via transverse.


namespace gc {
 

void collect();


template<typename T>
class ptr {
public:
    using element_type = T;

    ptr() noexcept : p(nullptr) {}
    ptr(std::nullptr_t) noexcept : p(nullptr) {}

    template<typename... Args>
    explicit ptr(std::in_place_t, Args&&... args) 
        : p(create_object(std::forward<Args>(args)...)) {}

    ptr(const ptr &other) noexcept : p(other.p) { if(p) ++p->ref_count; }
    ptr(ptr &&other) noexcept : p(other.p) { other.p = nullptr; }

    ptr &operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    ptr &operator=(const ptr &other) {
        reset();
        p = other.p;
        if(p)
            ++p->ref_count;
        return *this;
    }

    ptr &operator=(ptr &&other) {
        reset();
        p = other.p;
        other.p = nullptr;
        return *this;
    }

    ~ptr() { reset(); }

    explicit operator bool() const noexcept { return p; }

    T *get() const noexcept { return p ? &p->value : nullptr; }

    T *operator->() const noexcept { return get(); }
    T &operator*() const noexcept(!debug) { 
#ifdef DEBUG
        if(!p)
            throw std::logic_error("dereferencing null gc::ptr");
#endif
        return p->value; 
    }
    
    std::size_t use_count() const noexcept { return p ? p->ref_count : 0; }

    void swap(ptr &other) noexcept { std::swap(p, other.p); }

    void reset() {
        if(p && !detail::is_running && --p->ref_count == 0)
            p->free();
        p = nullptr;
    }

protected:
    template<typename U>
    friend struct detail::do_action;

    template<typename... Args>
    static detail::object<T> *create_object(Args&&... args) {
        try {
            return new detail::object<T>(std::forward<Args>(args)...);
        } catch(std::bad_alloc &) {
            debug_out("gc::collect on std::bad_alloc");
            gc::collect();
            return new detail::object<T>(std::forward<Args>(args)...);
        }
    }

    detail::object<T> *p;
};



template<typename T>
class anchor_ptr : public ptr<T>, public detail::anchor_node {
public:
    anchor_ptr() noexcept = default;
    anchor_ptr(std::nullptr_t) noexcept : ptr<T>(), detail::anchor_node() {}

    template<typename... Args>
    explicit anchor_ptr(std::in_place_t i, Args&&... args) : ptr<T>(i, std::forward<Args>(args)...), detail::anchor_node() {}

    anchor_ptr(ptr<T> p) noexcept : ptr<T>(std::move(p)), detail::anchor_node() {}

    anchor_ptr(const anchor_ptr &other) noexcept : ptr<T>(other), detail::anchor_node() {}
    anchor_ptr(anchor_ptr &&other) noexcept : ptr<T>(std::move(other)), detail::anchor_node() {}

    anchor_ptr &operator=(std::nullptr_t) {
        ptr<T>::operator=(nullptr);
        return *this;
    }

    anchor_ptr &operator=(ptr<T> other) {
        ptr<T>::operator=(std::move(other));
        return *this;
    }

    anchor_ptr &operator=(const anchor_ptr &other) {
        ptr<T>::operator=(other);
        return *this;
    }
    
    anchor_ptr &operator=(anchor_ptr &&other) {
        ptr<T>::operator=(std::move(other));
        return *this;
    }
    
    void swap(anchor_ptr &other) noexcept { ptr<T>::swap(other); }


    detail::node *detail_get_node() noexcept override { return ptr<T>::p; } 
};



template<typename T>
bool operator==(const ptr<T> &left, const ptr<T> &right) noexcept { return left.get() == right.get(); }

template<typename T>
bool operator!=(const ptr<T> &left, const ptr<T> &right) noexcept { return left.get() != right.get(); }

template<typename T>
bool operator<(const ptr<T> &left, const ptr<T> &right) noexcept { return left.get() < right.get(); }

template<typename T>
bool operator<=(const ptr<T> &left, const ptr<T> &right) noexcept { return left.get() <= right.get(); }

template<typename T>
bool operator>(const ptr<T> &left, const ptr<T> &right) noexcept { return left.get() > right.get(); }

template<typename T>
bool operator>=(const ptr<T> &left, const ptr<T> &right) noexcept { return left.get() >= right.get(); }



template<typename T, typename... Args>
ptr<T> make_ptr(Args&&... args) {
    return ptr<T>(std::in_place_t(), std::forward<Args>(args)...); 
}


template<typename T, typename... Args>
anchor_ptr<T> make_anchor_ptr(Args&&... args) {
    return anchor_ptr<T>(std::in_place_t(), std::forward<Args>(args)...); 
}


std::size_t object_count();
std::size_t anchor_count();

}  // namespace gc



namespace std {

    template<typename T>
    void swap(gc::ptr<T> &left, gc::ptr<T> &right) noexcept { left.swap(right); }

    template<typename T>
    void swap(gc::anchor_ptr<T> &left, gc::anchor_ptr<T> &right) noexcept { left.swap(right); }

    template<typename T>
    struct hash<gc::ptr<T>> {
        std::size_t operator()(const gc::ptr<T> &p) const { return std::hash<T*>()(p.get()); }
    };

    template<typename T>
    struct hash<gc::anchor_ptr<T>> {
        std::size_t operator()(const gc::anchor_ptr<T> &p) const { return std::hash<T*>()(p.get()); }
    };

}  // namespace std

#endif

