#ifndef LIPH_POLY_OBJ_H
#define LIPH_POLY_OBJ_H

#include <algorithm>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace liph { 


enum class behavior { immovable, throw_movable, throw_copyable, nothrow_movable, standard, nothrow_copyable };
/*
                    makes
movable , copyable, sense? , name
--------------------------------------
no      , no      , yes    , immovable
no      , throws  , no     ,
no      , nothrow , no     ,
throws  , no      , yes    , throw_movable
throws  , throws  , yes    , throw_copyable
throws  , nothrow , no     ,
nothrow , no      , yes    , nothrow_movable
nothrow , throws  , yes    , standard
nothrow , nothrow , yes    , nothrow_copyable
*/


template<typename... T>
constexpr std::size_t max_sizeof() { return std::max({sizeof(T)...}); }


template<typename T>
struct as_type { using type = T; };




template<bool>
struct copyable_trait {};

template<>
struct copyable_trait<false> {
    copyable_trait() = default;
    copyable_trait(const copyable_trait &) = delete;
    copyable_trait &operator=(const copyable_trait &) = delete;
    copyable_trait(copyable_trait &&) = default;
    copyable_trait &operator=(copyable_trait &&) = default;
};


template<bool>
struct movable_trait {};

template<>
struct movable_trait<false> {
    movable_trait() = default;
    movable_trait(const movable_trait &) = default;
    movable_trait &operator=(const movable_trait &) = default;
    movable_trait(movable_trait &&) = delete;
    movable_trait &operator=(movable_trait &&) = delete;
};




namespace detail {
    
    
template<behavior Behavior>
constexpr bool behavior_is_copyable = (Behavior == behavior::throw_copyable 
                                    || Behavior == behavior::nothrow_copyable 
                                    || Behavior == behavior::standard);

template<behavior Behavior>                            
constexpr bool behavior_is_nothrow_copyable = (Behavior == behavior::nothrow_copyable);

template<behavior Behavior>
constexpr bool behavior_is_movable          = (Behavior != behavior::immovable);

template<behavior Behavior>
constexpr bool behavior_is_nothrow_movable = (Behavior == behavior::nothrow_movable
                                           || Behavior == behavior::nothrow_copyable 
                                           || Behavior == behavior::standard);
                                           
                                           
constexpr std::size_t poly_obj_align(std::size_t n) {
    if(n >= alignof(std::max_align_t))
        return alignof(std::max_align_t);
        
    if(!(n & (n - 1)))  // if n is a power of 2
        return n;
    
    while(std::size_t next = (n & (n - 1))) // determine the highest bit
        n = next;
    
    return n << 1;  // round up
}



template<typename Base>
struct behavior_def {
    const Base *get(const char *data) { return get(const_cast<char*>(data)); }
    
    virtual Base *get(char *data) = 0;
    
    virtual void copy(const char *from, char *to) = 0;
    virtual void move(char *from, char *to) = 0;
    
    virtual void copy_assign(const char *from, char *to, Base *to_base, behavior_def<Base> **vptr) = 0;
    virtual void move_assign(char *from, char *to, Base *to_base, behavior_def<Base> **vptr) = 0;
};

template<typename Base, typename Derived>
struct behavior_impl : behavior_def<Base> {
    const Derived *get_derived(const char *data) {
        return std::launder(reinterpret_cast<const Derived*>(data));
    }
    
    Derived *get_derived(char *data) {
        return std::launder(reinterpret_cast<Derived*>(data));
    }
    
    Base *get(char *data) override { return get_derived(data); }
    
    void copy(const char *from, char *to) override {
        if constexpr(!std::is_copy_constructible_v<Derived>) {
            throw std::logic_error("Shouldn't be possible to call behavior_impl::copy with non-copyable Derived");
        } else {
            new (to) Derived(*get_derived(from)); 
        }
    }
    
    void move(char *from, char *to) override {
        if constexpr(!std::is_move_constructible_v<Derived> && !std::is_copy_constructible_v<Derived>) {
            throw std::logic_error("Shouldn't be possible to call behavior_impl::move with non-movable Derived");
        } else if constexpr(std::is_move_constructible_v<Derived>) {
            new (to) Derived (std::move(*get_derived(from)));
        } else {
            new (to) Derived (*get_derived(from));
        }
    }
    
    void copy_assign(const char *from, char *to, Base *to_base, behavior_def<Base> **vptr) override { 
        if(from == to)
            return;
        if constexpr(!std::is_copy_constructible_v<Derived>) {
            throw std::logic_error("Shouldn't be possible to call behavior_impl::copy_assign with non-copyable Derived");
        } else if constexpr(std::is_nothrow_copy_constructible_v<Derived> || !std::is_nothrow_move_constructible_v<Derived>) {
            if(to_base) to_base->~Base();
            try {
                new (to) Derived(*get_derived(from));
            } catch(...) {
                if(vptr) *vptr = nullptr;
                throw;
            }
        } else {
            alignas(alignof(Derived)) char temp_data[sizeof(Derived)];
            Derived *temp = new (temp_data) Derived(*get_derived(from));
            try {
                if(to_base) to_base->~Base();
                try {
                    new (to) Derived (std::move(*temp));
                } catch(...) {
                    if(vptr) *vptr = nullptr;
                    throw;
                }
            } catch(...) {
                temp->~Derived();
                throw;
            }
            temp->~Derived();
        }
    }
    
    void move_assign(char *from, char *to, Base *to_base, behavior_def<Base> **vptr) override {
        if(from == to)
            return;
        if constexpr(!std::is_move_constructible_v<Derived> && !std::is_copy_constructible_v<Derived>) {
            throw std::logic_error("Shouldn't be possible to call behavior_impl::move_assign with non-movable Derived");
        } else if constexpr(std::is_move_constructible_v<Derived> && (std::is_nothrow_move_constructible_v<Derived> || !std::is_nothrow_copy_constructible_v<Derived>)) {
            if(to_base) to_base->~Base();
            try {
                new (to) Derived (std::move(*get_derived(from)));
            } catch(...) {
                if(vptr) *vptr = nullptr;
                throw;
            }
        } else {
            copy(from, to, to_base);
        }
    }
    
    inline static behavior_impl<Base, Derived> obj;
};

    
template<typename Base, typename Derived, std::size_t MaxBytes, behavior B, bool FastPtr, typename... Args>
void poly_obj_construct(char *data, Args... args) {
    static_assert(std::is_base_of_v<Base, Derived>, "Derived is not derived from Base");
    static_assert(sizeof(Derived) <= MaxBytes, "Derived is too large");
    static_assert(poly_obj_align(MaxBytes) % alignof(Derived) == 0, "Alignment calculation was wrong");
    
    static_assert(!behavior_is_copyable<B> || std::is_copy_constructible_v<Derived>, "Derived must be copyable with this Behavior");
    static_assert(!behavior_is_nothrow_copyable<B> || std::is_nothrow_copy_constructible_v<Derived>, "Derived must be nothrow copyable with this Behavior");
    static_assert(!behavior_is_movable<B> || std::is_move_constructible_v<Derived>, "Derived must be movable with this Behavior");
    static_assert(!behavior_is_nothrow_movable<B> || std::is_nothrow_move_constructible_v<Derived>, "Derived must be nothrow movable with this Behavior");
    
    Derived *p = new (data) Derived(std::forward<Args>(args)...);
    
    if constexpr(FastPtr) {
        try { 
            if(static_cast<void*>(static_cast<Base*>(p)) != static_cast<void*>(data))
                throw std::logic_error("Base must be Derived's first base class with FastPtr == true");
        } catch(...) {
            p->~Derived();
            throw;
        }
    }
}


template<typename Base, std::size_t MaxBytes, behavior Behavior, bool FastPtr>
struct poly_obj_base {
    poly_obj_base() : vptr(nullptr) {}
    
    template<typename Derived>
    poly_obj_base(Derived &&derived) : poly_obj_base(as_type<Derived>(), std::forward<Derived>(derived)) {}
    
    template<typename Derived, typename... Args>
    poly_obj_base(as_type<Derived>, Args... args) : vptr(&behavior_impl<Base, Derived>::obj) {
        poly_obj_construct<Base, Derived, MaxBytes, Behavior, FastPtr>(data, std::forward<Args>(args)...);
    }
    
    poly_obj_base(const poly_obj_base &other)
        : vptr(other.vptr) { if(vptr) vptr->copy(other.data, data); }
        
    poly_obj_base(poly_obj_base &&other)
        : vptr(other.vptr) { if(vptr) vptr->move(other.data, data); }
        
    poly_obj_base &operator=(const poly_obj_base &other) {
        if(other.vptr) 
            other.vptr->copy_assign(other.data, data, ptr(), &vptr);
        else if(Base *p = ptr()) 
            p->~Base();
        vptr = other.vptr;
        return *this;
    }
    
    poly_obj_base &operator=(poly_obj_base &&other) {
        if(other.vptr) 
            other.vptr->move_assign(other.data, data, ptr(), &vptr);
        else if(Base *p = ptr()) 
            p->~Base();
        vptr = other.vptr;
        return *this;
    }

    Base *ptr() noexcept {
        if constexpr(FastPtr)
            return vptr ? std::launder(reinterpret_cast<Base*>(data)) : nullptr;
        else if(vptr)
            return vptr->get(data);
        else
            return nullptr;
    }
    
    const Base *ptr() const noexcept {
        if constexpr(FastPtr)
            return vptr ? std::launder(reinterpret_cast<const Base*>(data)) : nullptr;
        else if(vptr)
            return vptr->get(data);
        else
            return nullptr;
    }
    
    ~poly_obj_base() { if(Base *p = ptr()) p->~Base(); }
    
    behavior_def<Base> *vptr;
    alignas(poly_obj_align(MaxBytes)) char data[MaxBytes];
};

template<typename Base, std::size_t MaxBytes>
struct poly_obj_base<Base, MaxBytes, behavior::immovable, true> {
    poly_obj_base() {}
    
    template<typename Derived>
    poly_obj_base(Derived &&derived) : poly_obj_base(as_type<Derived>(), std::forward<Derived>(derived)) {}
    
    template<typename Derived, typename... Args>
    poly_obj_base(as_type<Derived>, Args... args) {
        poly_obj_construct<Base, Derived, MaxBytes, behavior::immovable, true>(data, std::forward<Args>(args)...);
    }
    
    Base *ptr() noexcept { return std::launder(reinterpret_cast<Base*>(data)); }
    
    const Base *ptr() const noexcept { return std::launder(reinterpret_cast<const Base*>(data)); }
    
    ~poly_obj_base() { ptr()->~Base(); }
    
    alignas(poly_obj_align(MaxBytes)) char data[MaxBytes];
};


}  // namespace detail



template<typename Base, std::size_t MaxBytes, behavior Behavior = behavior::standard, bool FastPtr = true>
class poly_obj 
    : private detail::poly_obj_base<Base, MaxBytes, Behavior, FastPtr>, 
      private copyable_trait<detail::behavior_is_copyable<Behavior>>, 
      private movable_trait<detail::behavior_is_movable<Behavior>>
{
    static_assert(std::has_virtual_destructor_v<Base>);
    static_assert(std::is_nothrow_destructible_v<Base>);
    
public:
    using reference = Base&;
    using const_reference = const Base&;
    using pointer = Base*;
    using const_pointer = const Base*;
    
    constexpr static bool is_copyable         = detail::behavior_is_copyable<Behavior>;
    constexpr static bool is_nothrow_copyable = detail::behavior_is_nothrow_copyable<Behavior>;
    constexpr static bool is_movable          = detail::behavior_is_movable<Behavior>;
    constexpr static bool is_nothrow_movable  = detail::behavior_is_nothrow_movable<Behavior>;
    
    using detail::poly_obj_base<Base, MaxBytes, Behavior, FastPtr>::poly_obj_base;
    using detail::poly_obj_base<Base, MaxBytes, Behavior, FastPtr>::ptr;
    
    
    poly_obj(const poly_obj &) noexcept(is_nothrow_copyable) = default;
    poly_obj(poly_obj &&) noexcept(is_nothrow_movable) = default;
    
    poly_obj &operator=(const poly_obj &) noexcept(is_nothrow_copyable) = default;
    poly_obj &operator=(poly_obj &&) noexcept(is_nothrow_movable) = default;
    
    
    template<behavior B = Behavior>
    poly_obj(std::enable_if_t<B != behavior::immovable || !FastPtr>* = nullptr) noexcept {}
    
    
    template<behavior B = Behavior>
    std::enable_if_t<B != behavior::immovable || !FastPtr> reset() noexcept { 
        if(Base *p = ptr()) p->~Base(); 
        this->vptr = nullptr; 
    }
    
    bool empty() const noexcept { return !ptr(); }
    
    
    Base &get() noexcept { return *ptr(); }
    const Base &get() const noexcept { return *ptr(); }

    Base &operator*() noexcept { return *ptr(); }
    const Base &operator*() const noexcept { return *ptr(); }
    
    Base *operator->() noexcept { return ptr(); }
    const Base *operator->() const noexcept { return ptr(); }
};


} // namespace liph

#endif

