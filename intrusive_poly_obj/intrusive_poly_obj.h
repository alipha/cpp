#ifndef LIPH_INTRUSIVE_POLY_H
#define LIPH_INTRUSIVE_POLY_H

#include <algorithm>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>


namespace liph {

template<typename... T>
constexpr std::size_t max_sizeof() { return std::max({sizeof(T)...}); }


template<typename T>
struct as_type { using type = T; };


template<bool>
struct copy_constructible_trait {};

template<>
struct copy_constructible_trait<false> {
    copy_constructible_trait() = default;
    copy_constructible_trait(const copy_constructible_trait &) = delete;
    copy_constructible_trait &operator=(const copy_constructible_trait &) = default;
    copy_constructible_trait(copy_constructible_trait &&) = default;
    copy_constructible_trait &operator=(copy_constructible_trait &&) = default;
};


template<bool>
struct move_constructible_trait {};

template<>
struct move_constructible_trait<false> {
    move_constructible_trait() = default;
    move_constructible_trait(const move_constructible_trait &) = default;
    move_constructible_trait &operator=(const move_constructible_trait &) = default;
    move_constructible_trait(move_constructible_trait &&) = delete;
    move_constructible_trait &operator=(move_constructible_trait &&) = default;
};

template<bool>
struct copy_assignable_trait {};

template<>
struct copy_assignable_trait<false> {
    copy_assignable_trait() = default;
    copy_assignable_trait(const copy_assignable_trait &) = default;
    copy_assignable_trait &operator=(const copy_assignable_trait &) = delete;
    copy_assignable_trait(copy_assignable_trait &&) = default;
    copy_assignable_trait &operator=(copy_assignable_trait &&) = default;
};


template<bool>
struct move_assignable_trait {};

template<>
struct move_assignable_trait<false> {
    move_assignable_trait() = default;
    move_assignable_trait(const move_assignable_trait &) = default;
    move_assignable_trait &operator=(const move_assignable_trait &) = default;
    move_assignable_trait(move_assignable_trait &&) = default;
    move_assignable_trait &operator=(move_assignable_trait &&) = delete;
};




namespace detail {
    
    
constexpr std::size_t poly_obj_align(std::size_t n) {
    if(n >= alignof(std::max_align_t))
        return alignof(std::max_align_t);
        
    if(!(n & (n - 1)))  // if n is a power of 2
        return n;
    
    while(std::size_t next = (n & (n - 1))) // determine the highest bit
        n = next;
    
    return n << 1;  // round up
}


template<typename T>
constexpr bool has_copy_to(long) { return false; }

template<typename T>
constexpr bool has_copy_to(int, decltype(std::declval<T>().copy_to(nullptr))* = nullptr) { return true; }

template<typename T>
constexpr bool has_move_to(long) { return false; }

template<typename T>
constexpr bool has_move_to(int, decltype(std::declval<T>().move_to(nullptr))* = nullptr) { return true; }

template<typename T>
constexpr bool is_copy_to_noexcept(long) { return false; }

template<typename T>
constexpr bool is_copy_to_noexcept(int, decltype(std::declval<T>().copy_to(nullptr))* = nullptr) { return noexcept(std::declval<T>().copy_to(nullptr)); }

template<typename T>
constexpr bool is_move_to_noexcept(long) { return false; }

template<typename T>
constexpr bool is_move_to_noexcept(int, decltype(std::declval<T>().move_to(nullptr))* = nullptr) { return noexcept(std::declval<T>().move_to(nullptr)); }

template<typename T>
constexpr bool is_moving_noexcept = (has_move_to<T>(0) && is_move_to_noexcept<T>(0)) || (!has_move_to<T>(0) && is_copy_to_noexcept<T>(0));


template<typename Base, std::size_t MaxBytes>
struct intrusive_poly_obj_base {
    template<typename Derived>
    intrusive_poly_obj_base(Derived &&derived) : intrusive_poly_obj_base(as_type<Derived>(), std::forward<Derived>(derived)) {}
    
    template<typename Derived, typename... Args>
    intrusive_poly_obj_base(as_type<Derived>, Args... args) {
        static_assert(std::is_base_of_v<Base, Derived>, "Derived is not derived from Base");
        static_assert(sizeof(Derived) <= MaxBytes, "Derived is too large");
        static_assert(detail::poly_obj_align(MaxBytes) % alignof(Derived) == 0, "Alignment calculation was wrong");
                
        Derived *p = new (data) Derived(std::forward<Args>(args)...);
        
        try {
            if(static_cast<void*>(static_cast<Base*>(p)) != static_cast<void*>(data))
                throw std::logic_error("Base must be Derived's first base class");
        } catch(...) {
            p->~Derived();
            throw;
        }
    }
    
    
    intrusive_poly_obj_base(const intrusive_poly_obj_base &other) { other.ptr()->copy_to(data); }
    
    intrusive_poly_obj_base(intrusive_poly_obj_base &&other) {
        if constexpr(has_move_to<Base>(0)) {
            other.ptr()->move_to(data);
        } else {
            other.ptr()->copy_to(data);
        }
    }
    
    intrusive_poly_obj_base &operator=(const intrusive_poly_obj_base &other) {
        if(this == &other)
            return *this;
        
        if constexpr(is_copy_to_noexcept<Base>(0) || !is_move_to_noexcept<Base>(0)) {
            ptr()->~Base();
            other.ptr()->copy_to(data);
        } else {
            intrusive_poly_obj_base temp(other);
            *this = std::move(temp);
        }
        return *this;
    }
    
    intrusive_poly_obj_base &operator=(intrusive_poly_obj_base &&other) {
        if(this == &other)
            return *this;
        
        if constexpr(has_move_to<Base>(0) && (!is_copy_to_noexcept<Base>(0) || is_move_to_noexcept<Base>(0))) {
            ptr()->~Base();
            other.ptr()->move_to(data);
        } else {
             *this = other;
        }
        return *this;
    }
    
        
    Base *ptr() noexcept { return std::launder(reinterpret_cast<Base*>(data)); }
    
    const Base *ptr() const noexcept { return std::launder(reinterpret_cast<const Base*>(data)); }
    
    ~intrusive_poly_obj_base() { ptr()->~Base(); }

private:
    alignas(poly_obj_align(MaxBytes)) char data[MaxBytes];
};
    

}  // namespace detail



template<typename Base, typename Derived, bool Copyable = detail::has_copy_to<Base>(0), bool Movable = detail::has_move_to<Base>(0)>
struct implement_poly_obj : Base { using Base::Base; };


template<typename Base, typename Derived>
struct implement_poly_obj<Base, Derived, false, true> : Base {
    using Base::Base;
    
    void move_to(void *dest) noexcept(noexcept(std::declval<Base>().move_to(nullptr))) override {
        new (dest) Derived(std::move(*static_cast<Derived*>(this)));
    }
};


template<typename Base, typename Derived>
struct implement_poly_obj<Base, Derived, true, false> : Base {
    using Base::Base;
    
    void copy_to(void *dest) const noexcept(noexcept(std::declval<Base>().copy_to(nullptr))) override { 
        new (dest) Derived(*static_cast<const Derived*>(this));
    }
};


template<typename Base, typename Derived>
struct implement_poly_obj<Base, Derived, true, true> : Base {
    using Base::Base;
    
    void copy_to(void *dest) const noexcept(noexcept(std::declval<Base>().copy_to(nullptr))) override { 
        new (dest) Derived(*static_cast<const Derived*>(this));
    }
    
    void move_to(void *dest) noexcept(noexcept(std::declval<Base>().move_to(nullptr))) override {
        new (dest) Derived(std::move(*static_cast<Derived*>(this)));
    }
};



template<typename Base, std::size_t MaxBytes>
class intrusive_poly_obj 
    : private detail::intrusive_poly_obj_base<Base, MaxBytes>,
      private copy_constructible_trait<detail::has_copy_to<Base>(0)>,
      private move_constructible_trait<detail::has_copy_to<Base>(0) || detail::has_move_to<Base>(0)>,
      private copy_assignable_trait<detail::has_copy_to<Base>(0) && (detail::is_copy_to_noexcept<Base>(0) || detail::is_move_to_noexcept<Base>(0))>,
      private move_assignable_trait<detail::is_copy_to_noexcept<Base>(0) || detail::is_move_to_noexcept<Base>(0)>
{
    static_assert(std::has_virtual_destructor_v<Base>);
    static_assert(std::is_nothrow_destructible_v<Base>);
    
public:
    using reference = Base&;
    using const_reference = const Base&;
    using pointer = Base*;
    using const_pointer = const Base*;

    using detail::intrusive_poly_obj_base<Base, MaxBytes>::intrusive_poly_obj_base;
    using detail::intrusive_poly_obj_base<Base, MaxBytes>::ptr;
    
    
    intrusive_poly_obj(const intrusive_poly_obj &) noexcept(detail::is_copy_to_noexcept<Base>(0)) = default;
    intrusive_poly_obj(intrusive_poly_obj &&) noexcept(detail::is_moving_noexcept<Base>) = default;
    
    intrusive_poly_obj &operator=(const intrusive_poly_obj &) noexcept(detail::is_copy_to_noexcept<Base>(0)) = default;
    intrusive_poly_obj &operator=(intrusive_poly_obj &&) noexcept(detail::is_moving_noexcept<Base>) = default;  
    
    
    Base &get() noexcept { return *ptr(); }
    const Base &get() const noexcept { return *ptr(); }

    Base &operator*() noexcept { return *ptr(); }
    const Base &operator*() const noexcept { return *ptr(); }
    
    Base *operator->() noexcept { return ptr(); }
    const Base *operator->() const noexcept { return ptr(); }
};


}  // namespace liph
#endif

