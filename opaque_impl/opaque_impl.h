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


template<typename Impl, std::size_t Size, std::size_t Align = Size>
struct opaque_impl {
    static_assert(Align > 0, "Align must be non-zero");
    
    opaque_impl(const opaque_impl &other) noexcept(noexcept(Impl(std::declval<const Impl&>()))) : opaque_impl(*other) {}
    opaque_impl(opaque_impl &&other) noexcept(noexcept(Impl(std::declval<Impl&&>()))) : opaque_impl(std::move(*other)) {}

    template<typename... Args>
    explicit opaque_impl(Args&&... args) noexcept(noexcept(Impl(std::declval<Args&&>()...))) {
        validate();   
        new (storage) Impl(std::forward<Args>(args)...);
    }
    
    template<typename T, std::enable_if_t<std::is_constructible_v<Impl, T>, int> = 0>
    explicit opaque_impl(std::initializer_list<T> il) 
            noexcept(noexcept(Impl(std::declval<std::initializer_list<T>>()))) {
        validate();   
        new (storage) Impl(il);
    }

    opaque_impl &operator=(const opaque_impl &other) noexcept(noexcept(*this = *other)) { return *this = *other; }
    opaque_impl &operator=(opaque_impl &&other) noexcept(noexcept(*this = std::move(*other))) { return *this = std::move(*other); }

    template<typename Arg>
    opaque_impl &operator=(Arg &&arg) noexcept(noexcept(this->value() = std::declval<Arg&&>())) { 
        value() = std::forward<Arg>(arg);
        return *this;
    }
    
    Impl &value() noexcept { return *reinterpret_cast<Impl*>(storage); }
    const Impl &value() const noexcept { return *reinterpret_cast<const Impl*>(storage); }

    Impl &operator*() noexcept { return value(); }
    const Impl &operator*() const noexcept { return value(); }

    Impl *operator->() noexcept { return &value(); }
    const Impl *operator->() const noexcept { return &value(); }

    ~opaque_impl() { value().~Impl(); }
    
private:
    constexpr void validate() noexcept {
        static_assert(sizeof(Impl) <= Size, "Size is not large enough for sizeof(Impl)");
        static_assert(detail::calc_alignment(Align) % alignof(Impl) == 0, "Alignment is not appropriate for Impl");
    }
    
    alignas(detail::calc_alignment(Align)) char storage[Size];
};

#endif
