/*
 * liph::optional has a nearly identical interface to std::optional (and converts to/from implicitly),
 * except that liph::optional<T&> is well-defined.
 *
 * liph::optional<T> (where T is not a reference) is just an alias for std::optional<T>.
 *
 * Copy construction and assignment of liph::optional<T&> performs rebinding of the reference.
 */
#ifndef LIPH_OPTIONAL_HPP
#define LIPH_OPTIONAL_HPP

#include <optional>
#include <typeindex>
#include <type_traits>
#include <utility>


namespace liph {
    template<typename T>
    class optional_ref {
    public:
        using value_type = T&;
        
        constexpr optional_ref() noexcept : ptr() {}
        constexpr optional_ref( std::nullopt_t ) noexcept : ptr() {}

        constexpr optional_ref( const optional_ref& other ) noexcept = default;

        constexpr optional_ref( optional_ref&& other ) noexcept : ptr(other.ptr) { other.ptr = nullptr; }

        template < class U >
        constexpr optional_ref( const optional_ref<U>& other ) noexcept : ptr(other.ptr) {}  // TODO: conditionally explicit?
        
        template < class U >
        constexpr optional_ref( optional_ref<U>&& other ) noexcept : ptr(other.ptr) { other.ptr = nullptr; }  // TODO: conditionally explicit?
        
        template < class U >
        constexpr optional_ref( const std::optional<U>& other ) noexcept : ptr(other ? &*other : nullptr) {}  // TODO: conditionally explicit?
        
        template < class U >
        constexpr optional_ref( std::optional<U>& other ) noexcept : ptr(other ? &*other : nullptr) {}  // TODO: conditionally explicit?

        template < class U >
        constexpr optional_ref( U& value, std::enable_if_t<std::is_convertible_v<U*, T*>>* = 0 ) noexcept : ptr(&value) {}  // TODO: conditionally explicit?

        
        constexpr optional_ref& operator=( std::nullopt_t ) noexcept { ptr = nullptr; return *this; }

        constexpr optional_ref& operator=( const optional_ref& other ) noexcept = default;

        constexpr optional_ref& operator=( optional_ref&& other ) noexcept {
            ptr = other.ptr;
            other.ptr = nullptr;
            return *this;
        }

        template< class U >
        constexpr optional_ref& operator=( U& value ) noexcept { ptr = &value; return *this; }

        template< class U >
        constexpr optional_ref& operator=( const optional_ref<U>& other ) noexcept { ptr = other.ptr; return *this; }

        template< class U >
        constexpr optional_ref& operator=( optional_ref<U>&& other ) noexcept {
            ptr = other.ptr;
            other.ptr = nullptr;
            return *this;
        }
        
        template< class U >
        constexpr optional_ref& operator=( const std::optional<U>& other ) noexcept { ptr = other ? &*other : nullptr; return *this; }
        
        template< class U >
        constexpr optional_ref& operator=( std::optional<U>& other ) noexcept { ptr = other ? &*other : nullptr; return *this; }
        
        
        constexpr T* operator->() const noexcept { return ptr; }
        constexpr T& operator*() const noexcept { return *ptr; }
        
        
        constexpr explicit operator bool() const noexcept { return ptr; }
        constexpr bool has_value() const noexcept { return ptr; }
        
        template<typename U>
        constexpr operator std::optional<U>() const noexcept {
            return std::optional<U>(ptr ? *ptr : std::nullopt);
        }
        
        constexpr T& value() const {
            if(ptr)
                return *ptr;
            throw std::bad_optional_access();
        }
        
        template< class U >
        constexpr T& value_or( U& default_value ) const noexcept { return ptr ? *ptr : default_value; }
        
        template< class U >
        constexpr T& emplace( U& value ) noexcept { return ptr = &value; }
        
        constexpr void swap( optional_ref& other ) noexcept { std::swap(ptr, other.ptr); }
        
        constexpr void reset() noexcept { ptr = nullptr; }
        
    private:
        T *ptr;
    };
    
    
    namespace detail {
        template<typename, typename T>
        const T *get_ptr(const T &obj) { return &obj; }
        
        template<typename, typename T>
        const T *get_ptr(const std::optional<T> &opt) { return opt ? &*opt : nullptr; }
        
        template<typename T>
        const T *get_ptr(std::nullopt_t) { return nullptr; }
    }
    
    
    template< class T, class U >
    constexpr bool operator==( optional_ref<T> lhs, optional_ref<U> rhs ) noexcept {
        return lhs.operator->() == rhs.operator->();
    }
    
    template< class T, class U >
    constexpr bool operator<( optional_ref<T> lhs, optional_ref<U> rhs ) noexcept {
        return std::less<std::conditional_t<std::is_convertible_v<const T*, const U*>, const U*, const T*>>()(lhs.operator->(), rhs.operator->());
    }
   
    
    template< class T, class U >
    constexpr bool operator==( optional_ref<T> lhs, U& rhs ) noexcept {
        return lhs.operator->() == detail::get_ptr<T>(rhs);
    }
    
    template< class T, class U >
    constexpr bool operator<( optional_ref<T> lhs, U& rhs ) noexcept {
        auto p = detail::get_ptr<T>(rhs);
        return std::less<std::conditional_t<std::is_convertible_v<const T*, decltype(p)>, decltype(p), const T*>>()(lhs.operator->(), p);
    }
    
    template< class T, class U >
    constexpr bool operator==( T& lhs, optional_ref<U> rhs ) noexcept {
        return detail::get_ptr<U>(lhs) == rhs.operator->();
    }
    
    template< class T, class U >
    constexpr bool operator<( T& lhs, optional_ref<U> rhs ) noexcept {
        auto p = detail::get_ptr<U>(lhs);
        return std::less<std::conditional_t<std::is_convertible_v<decltype(p), const U*>, const U*, decltype(p)>>()(p, rhs.operator->());
    }
    
    
    template< class T, class U >
    constexpr bool operator>( optional_ref<T> lhs, optional_ref<U> rhs ) noexcept { return rhs < lhs; }
    
    template< class T, class U >
    constexpr bool operator<=( optional_ref<T> lhs, const optional_ref<U> rhs ) noexcept { return !(lhs > rhs); }
    
    template< class T, class U >
    constexpr bool operator>=( optional_ref<T> lhs, const optional_ref<U> rhs ) noexcept { return !(lhs < rhs); }
    
    template< class T, class U >
    constexpr bool operator!=( optional_ref<T> lhs, const optional_ref<U> rhs ) noexcept { return !(lhs == rhs); }
    
    
    template< class T, class U >
    constexpr bool operator>( T& lhs, const optional_ref<U> rhs ) noexcept { return rhs < lhs; }
    
    template< class T, class U >
    constexpr bool operator<=( T& lhs, const optional_ref<U> rhs ) noexcept { return !(lhs > rhs); }
    
    template< class T, class U >
    constexpr bool operator>=( T& lhs, const optional_ref<U> rhs ) noexcept { return !(lhs < rhs); }
    
    template< class T, class U >
    constexpr bool operator!=( T& lhs, const optional_ref<U> rhs ) noexcept { return !(lhs == rhs); }
    
    
    template< class T, class U >
    constexpr bool operator>( optional_ref<T> lhs, U& rhs ) noexcept { return rhs < lhs; }
    
    template< class T, class U >
    constexpr bool operator<=( optional_ref<T> lhs, U& rhs ) noexcept { return !(lhs > rhs); }
    
    template< class T, class U >
    constexpr bool operator>=( optional_ref<T> lhs, U& rhs ) noexcept { return !(lhs < rhs); }
    
    template< class T, class U >
    constexpr bool operator!=( optional_ref<T> lhs, U& rhs ) noexcept { return !(lhs == rhs); }
    
    
    
    template<typename T>
    using optional = std::conditional_t<std::is_reference_v<T>, optional_ref<std::remove_reference_t<T>>, std::optional<T>>;
}

namespace std {
  template <typename T> struct hash<liph::optional_ref<T>>
  {
    constexpr size_t operator()(const liph::optional_ref<T> & r) const noexcept
    {
        return hash<T*>()(r ? &*r : nullptr);
    }
  };
}

#endif

