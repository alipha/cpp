#ifndef LIPH_GC_HPP
#define LIPH_GC_HPP

#include "gc_detail.hpp"
#include <functional>
#include <new>
#include <typeinfo>

// TODO: make collect() and iterate_from not use global temp_head
// TODO: const correctness?
// TODO: use allocators?
// TODO: exception safe
// TODO: support T[]
// TODO: gc::anchor
// TODO: weak_ptr
// TODO: on gc::ptr constructor, call transverse and check
//       that all gc::ptrs that have been created are
//       reached via transverse.


namespace gc {
 

std::size_t object_count();
std::size_t anchor_count();

std::size_t get_memory_used();
std::size_t get_memory_limit();
void set_memory_limit(std::size_t limit);


template<typename T>
class ptr;


template<typename... Types>
struct for_types {};

template<>
struct for_types<> {
    template<typename Func>
    static void iterate_all_objects(Func &&func) {
        static_assert(sizeof(Func) && false, "iterate_all_objects must be called with at least one type");
    }

    template<typename Func>
    static bool run_on_node(detail::node *, Func &&) { return false; }
};


template<typename Type, typename... Types>
struct for_types<Type, Types...> {
private:
    template<typename Func>
    struct custom_action : action {

        custom_action(Func &func) : func(func) {}

        // returning true means: did i do something? false means it was already marked
        bool detail_perform(detail::node *node) override { 
            if(debug && !node)
                throw std::logic_error("custom_action: null");
            node->mark_reachable(false);
            run_on_node(node, func);
            return true;
        }

        Func &func;
    };

public:
    template<typename Func>
    static void iterate_all_objects(Func &&func) {
        detail::node *n = detail::active_head.next;

        while(n != &detail::active_head) {
            run_on_node(n, std::forward<Func>(func));    
            n = n->next;
        }
    }

    
    template<typename T, typename Func>
    static void iterate_from(ptr<T> &p, Func &&func) {
        custom_action<Func> act(func);
        detail::transverse_and_mark_reachable(p.n, act);
        debug_out("resetting reachable flag");
        detail::reset_reachable_flag(detail::temp_head);
        detail::move_temp_to_active();
    }

private:
    template<typename... Args>
    friend struct for_types;

    template<typename Func>
    static bool run_on_node(detail::node *n, Func &&func) {
        if(typeid(detail::object<Type>&) == typeid(*n)) {
            func(static_cast<detail::object<Type>*>(n)->value);
            return true;
        } else {
            bool matched = for_types<Types...>::run_on_node(n, std::forward<Func>(func));
            if constexpr(std::is_same_v<Type, void*>) {
                if(!matched) {
                    func(n->get_value());
                    return true;
                }
            }
            return matched;
        }
    }
};


template<typename T>
class ptr {
public:
    using element_type = T;

    ptr() noexcept : n(nullptr), p(nullptr) {}
    ptr(std::nullptr_t) noexcept : n(nullptr), p(nullptr)  {}

    template<typename... Args>
    explicit ptr(std::in_place_t, Args&&... args) : n(nullptr), p(nullptr) {
        detail::object<T> *obj = create_object(std::forward<Args>(args)...);
        n = obj;
        p = &obj->value;
    }

    ptr(const ptr &other) noexcept : n(other.n), p(other.p) { if(n) ++n->ref_count; }

    ptr(ptr &&other) noexcept : n(other.n), p(other.p) { 
        other.n = nullptr;
        other.p = nullptr;
    }

    template<typename U>
    ptr(ptr<U> other) noexcept : n(other.n), p(other.p) {
        other.n = nullptr;
        other.p = nullptr;
    }

    ptr &operator=(std::nullptr_t) {
        reset();
        return *this;
    }

    ptr &operator=(const ptr &other) {
        reset();
        n = other.n;
        p = other.p;
        if(n)
            ++n->ref_count;
        return *this;
    }

    ptr &operator=(ptr &&other) {
        reset();
        n = other.n;
        p = other.p;
        other.n = nullptr;
        other.p = nullptr;
        return *this;
    }

    template<typename U>
    ptr &operator=(ptr<U> other) {
        reset();
        n = other.n;
        p = other.p;
        other.n = nullptr;
        other.p = nullptr;
        return *this;
    }

    ~ptr() { reset(); }

    explicit operator bool() const noexcept { return p; }

    T *get() const noexcept { return p; }

    T *operator->() const noexcept { return get(); }
    T &operator*() const noexcept(!debug) { 
#ifdef DEBUG
        if(!p)
            throw std::logic_error("dereferencing null gc::ptr");
#endif
        return *p; 
    }
    
    std::size_t use_count() const noexcept { return n ? n->ref_count : 0; }

    void swap(ptr &other) noexcept {
        std::swap(n, other.n);
        std::swap(p, other.p);
    }

    void reset() {
        if(n && !detail::is_running && --n->ref_count == 0)
            n->free();
        n = nullptr;
        p = nullptr;
    }

protected:
    template<typename U>
    friend class ptr;
    
    template<typename V, typename U>
    friend ptr<V> static_pointer_cast(ptr<U> p) noexcept;

    template<typename V, typename U>
    friend ptr<V> dynamic_pointer_cast(ptr<U> p) noexcept;

    template<typename V, typename U>
    friend ptr<V> const_pointer_cast(ptr<U> p) noexcept;

    template<typename V, typename U>
    friend ptr<V> reinterpret_pointer_cast(ptr<U> p) noexcept;

    template<typename U>    
    friend struct detail::do_action;

    template<typename... Types>
    friend struct for_types;


    ptr(detail::node *n, T *p) noexcept : n(n), p(p) { if(n) ++n->ref_count; }


    template<typename... Args>
    static detail::object<T> *create_object(Args&&... args) {
        return detail::create_object<T>(std::forward<Args>(args)...);
    }
    

    detail::node *n; 
    T *p;
};



template<typename T>
class anchor_ptr : public ptr<T>, public detail::anchor_node {
public:
    anchor_ptr() noexcept = default;
    anchor_ptr(std::nullptr_t) noexcept : ptr<T>(), detail::anchor_node() {}

    template<typename... Args>
    explicit anchor_ptr(std::in_place_t i, Args&&... args) : ptr<T>(i, std::forward<Args>(args)...), detail::anchor_node() {}

    template<typename U>
    anchor_ptr(ptr<U> p) noexcept : ptr<T>(std::move(p)), detail::anchor_node() {}

    anchor_ptr(const anchor_ptr &other) noexcept : ptr<T>(other), detail::anchor_node() {}
    anchor_ptr(anchor_ptr &&other) noexcept : ptr<T>(std::move(other)), detail::anchor_node() {}
    
    template<typename U>
    anchor_ptr(anchor_ptr<U> other) noexcept : ptr<T>(std::move(other)), detail::anchor_node() {}

    anchor_ptr &operator=(std::nullptr_t) {
        ptr<T>::operator=(nullptr);
        return *this;
    }

    template<typename U>
    anchor_ptr &operator=(ptr<U> other) {
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
    
    template<typename U>
    anchor_ptr &operator=(anchor_ptr<U> other) {
        ptr<T>::operator=(std::move(other));
        return *this;
    }

    void swap(anchor_ptr &other) noexcept { ptr<T>::swap(other); }


    detail::node *detail_get_node() noexcept override { return ptr<T>::n; } 
    
private:
    anchor_ptr(detail::node *n, T *p) noexcept : ptr<T>(n, p), detail::anchor_node() {}

    template<typename V, typename U>
    friend anchor_ptr<V> static_pointer_cast(anchor_ptr<U> p) noexcept;

    template<typename V, typename U>
    friend anchor_ptr<V> dynamic_pointer_cast(anchor_ptr<U> p) noexcept;

    template<typename V, typename U>
    friend anchor_ptr<V> const_pointer_cast(anchor_ptr<U> p) noexcept;

    template<typename V, typename U>
    friend anchor_ptr<V> reinterpret_pointer_cast(anchor_ptr<U> p) noexcept;
};



template<typename T>
struct allocator {
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    allocator() noexcept = default;
    
    template<typename U>
    allocator(const allocator<U> &) noexcept {}

    template<typename U>
    allocator &operator=(const allocator<U> &) noexcept { return *this; }

    T *allocate(std::size_t n) { return detail::allocate<T>(n, true); }

    void deallocate(T *p, std::size_t n) {
        std::allocator<T>().deallocate(p, n);
        detail::memory_used -= sizeof(T) * n + 8;
    }
};

template<typename T, typename U>
bool operator==(allocator<T>, allocator<U>) { return true; }

template<typename T, typename U>
bool operator!=(allocator<T>, allocator<U>) { return false; }



template<typename T, typename U>
bool operator==(const ptr<T> &left, const ptr<U> &right) noexcept { return left.get() == right.get(); }

template<typename T, typename U>
bool operator!=(const ptr<T> &left, const ptr<U> &right) noexcept { return left.get() != right.get(); }

template<typename T, typename U>
bool operator<(const ptr<T> &left, const ptr<U> &right) noexcept { return left.get() < right.get(); }

template<typename T, typename U>
bool operator<=(const ptr<T> &left, const ptr<U> &right) noexcept { return left.get() <= right.get(); }

template<typename T, typename U>
bool operator>(const ptr<T> &left, const ptr<U> &right) noexcept { return left.get() > right.get(); }

template<typename T, typename U>
bool operator>=(const ptr<T> &left, const ptr<U> &right) noexcept { return left.get() >= right.get(); }

template<typename T>
bool operator==(std::nullptr_t, const ptr<T> &right) noexcept { return !right.get(); }

template<typename T>
bool operator!=(std::nullptr_t, const ptr<T> &right) noexcept { return right.get(); }

template<typename T>
bool operator<(std::nullptr_t, const ptr<T> &right) noexcept { return right.get(); }

template<typename T>
bool operator<=(std::nullptr_t, const ptr<T> &) noexcept { return true; }

template<typename T>
bool operator>(std::nullptr_t, const ptr<T> &) noexcept { return false; }

template<typename T>
bool operator>=(std::nullptr_t, const ptr<T> &right) noexcept { return !right.get(); }

template<typename T>
bool operator==(const ptr<T> &left, std::nullptr_t) noexcept { return !left.get(); }

template<typename T>
bool operator!=(const ptr<T> &left, std::nullptr_t) noexcept { return left.get(); }

template<typename T>
bool operator<(const ptr<T> &, std::nullptr_t) noexcept { return false; }

template<typename T>
bool operator<=(const ptr<T> &left, std::nullptr_t) noexcept { return !left.get(); }

template<typename T>
bool operator>(const ptr<T> &left, std::nullptr_t) noexcept { return left.get(); }

template<typename T>
bool operator>=(const ptr<T> &, std::nullptr_t) noexcept { return true; }



template<typename T, typename... Args>
ptr<T> make_ptr(Args&&... args) {
    return ptr<T>(std::in_place_t(), std::forward<Args>(args)...); 
}


template<typename T, typename... Args>
anchor_ptr<T> make_anchor_ptr(Args&&... args) {
    return anchor_ptr<T>(std::in_place_t(), std::forward<Args>(args)...); 
}



template<typename T, typename U>
ptr<T> static_pointer_cast(ptr<U> p) noexcept { return ptr<T>(p.n, static_cast<T*>(p.p)); }

template<typename T, typename U>
ptr<T> dynamic_pointer_cast(ptr<U> p) noexcept { 
    T *tp = dynamic_cast<T*>(p.p);
    return ptr<T>(tp ? p.n : nullptr, tp); 
}

template<typename T, typename U>
ptr<T> const_pointer_cast(ptr<U> p) noexcept { return ptr<T>(p.n, const_cast<T*>(p.p)); }

template<typename T, typename U>
ptr<T> reinterpret_pointer_cast(ptr<U> p) noexcept { return ptr<T>(p.n, reinterpret_cast<T*>(p.p)); }


template<typename T, typename U>
anchor_ptr<T> static_pointer_cast(anchor_ptr<U> p) noexcept { return anchor_ptr<T>(p.n, static_cast<T*>(p.p)); }

template<typename T, typename U>
anchor_ptr<T> dynamic_pointer_cast(anchor_ptr<U> p) noexcept { 
    T *tp = dynamic_cast<T*>(p.p);
    return anchor_ptr<T>(tp ? p.n : nullptr, tp); 
}

template<typename T, typename U>
anchor_ptr<T> const_pointer_cast(anchor_ptr<U> p) noexcept { return anchor_ptr<T>(p.n, const_cast<T*>(p.p)); }

template<typename T, typename U>
anchor_ptr<T> reinterpret_pointer_cast(anchor_ptr<U> p) noexcept { return anchor_ptr<T>(p.n, reinterpret_cast<T*>(p.p)); }



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

