//
// opaque_impl is a class to assist with creating a "pimpl idiom"-like design.
// Traditional pimpl would use a std::unique_ptr to the implementation, but that
// requires an additional pointer indirection and dynamic allocation.
// 
// opaque_impl makes a compromise by exposing the byte size of the implmentation,
// but this allows it to avoid pointer indirection and dynamic allocation
// 
#ifndef OPAQUE_IMPL_H
#define OPAQUE_IMPL_H

#include <cstddef>
#include <initializer_list>
#include <new>
#include <type_traits>
#include <utility>


namespace detail {
    // if you provide an invalid alignment such as 6, then round up to the nearest power of 2.
    constexpr std::size_t calc_alignment(std::size_t provided) {
        if(provided > alignof(std::max_align_t))
            return alignof(std::max_align_t);
        
        if(provided & (provided - 1)) {
            while(provided & (provided - 1))
                provided &= provided - 1;
            return provided * 2;
        } else {
            return provided;
        }
    }
}


template<std::size_t Size, std::size_t Align = Size>
struct opaque_impl {
    static_assert(Align > 0, "Align must be non-zero");
    
    opaque_impl(const opaque_impl &) = delete;
    opaque_impl &operator=(const opaque_impl &) = delete;
    
    template<typename Impl>
    constexpr opaque_impl(Impl &&impl) noexcept(noexcept(Impl(std::declval<Impl>()))) {
        using T = std::remove_reference_t<Impl>;
        validate<T>();  
        new (storage) T(std::forward<Impl>(impl));
    }
    
    template<typename Impl, typename... Args>
    constexpr explicit opaque_impl(std::in_place_type_t<Impl>, Args&&... args) noexcept(noexcept(Impl(std::declval<Args>()...))) {
        validate<Impl>();   
        new (storage) Impl(std::forward<Args>(args)...);
    }
    
    template<typename Impl, typename U, typename... Args>
    constexpr explicit opaque_impl(std::in_place_type_t<Impl>, std::initializer_list<U> il, Args&&... args) 
            noexcept(noexcept(Impl(std::declval<std::initializer_list<U>>(), std::declval<Args>()...))) {
        validate<Impl>();   
        new (storage) Impl(il, std::forward<Args>(args)...);
    }
    
    template<typename Impl>
    Impl &get() noexcept { return *std::launder(reinterpret_cast<Impl*>(storage)); }
    
    template<typename Impl>
    const Impl &get() const noexcept { return *std::launder(reinterpret_cast<const Impl*>(storage)); }
    
private:
    template<typename Impl>
    constexpr void validate() noexcept {
        static_assert(sizeof(Impl) <= Size, "Size is not large enough for sizeof(Impl)");
        static_assert(detail::calc_alignment(Align) % alignof(Impl) == 0, "Alignment is not appropriate for Impl");
    }
    
    alignas(detail::calc_alignment(Align)) char storage[Size];
};

#endif

