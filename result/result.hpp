#ifndef LIPH_RESULT_HPP
#define LIPH_RESULT_HPP

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>


namespace liph {


struct bad_result_access : std::logic_error {
    bad_result_access(const std::string &msg) : std::logic_error(msg) {}
};

struct bad_result_value_access : bad_result_access {
    bad_result_value_access() : bad_result_access("result does not contain a value") {}
};

struct bad_result_error_access : bad_result_access {
    bad_result_error_access() : bad_result_access("result does not contain an error") {}
};


struct result_in_place_value {};
struct result_in_place_error {};



template<typename T, typename Error>
class result;

template<typename U>
constexpr bool is_result = false;

template<typename U, typename V>
constexpr bool is_result<result<U, V>> = true;



template<typename T, typename Error>
class result {
private:
    template<typename U, typename V>
    friend class result;
    
public:
    using value_type = T;
    using error_type = Error;
        
    
    template<typename U, typename V>
    explicit result(const result<U, V> &other) : val(
        other 
            ? std::variant<T, Error>(std::in_place_index_t<0>(), other.value())
            : std::variant<T, Error>(std::in_place_index_t<1>(), other.error())
    ) {}
    
    template<typename U, typename V>
    explicit result(result<U, V> &&other) : val(
        other 
            ? std::variant<T, Error>(std::in_place_index_t<0>(), std::move(other.value()))
            : std::variant<T, Error>(std::in_place_index_t<1>(), std::move(other.error()))
    ) {}
    
    template<typename U, typename = std::enable_if_t<!is_result<std::decay_t<U>>>>
    constexpr result(U &&v) : val(std::forward<U>(v)) {}
    
    
    template<typename... Args>
    constexpr explicit result(result_in_place_value, Args&&... args) 
            : val(std::in_place_index_t<0>(), std::forward<Args>(args)...) {}
            
    template<typename... Args>
    constexpr explicit result(result_in_place_error, Args&&... args) 
            : val(std::in_place_index_t<1>(), std::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    constexpr result(result_in_place_value, std::initializer_list<U> list, Args&&... args)
            : val(std::in_place_index_t<0>(), list, std::forward<Args>(args)...) {}
            
    template<typename U, typename... Args>
    constexpr result(result_in_place_error, std::initializer_list<U> list, Args&&... args)
            : val(std::in_place_index_t<1>(), list, std::forward<Args>(args)...) {}
    
    
    template<typename U> 
    constexpr result &operator=(U &&v) {
        val = std::forward<U>(v);
        return *this;
    }
    
    
    template<typename... Args>
    T &emplace_value(Args&&... args) {
        return val.template emplace<0>(std::forward<Args>(args)...);
    }
    
    template<typename U, typename... Args>
    T &emplace_value(std::initializer_list<U> list, Args&&... args) {
        return val.template emplace<0>(list, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    Error &emplace_error(Args&&... args) {
        return val.template emplace<1>(std::forward<Args>(args)...);
    }

    template<typename U, typename... Args>
    Error &emplace_error(std::initializer_list<U> list, Args&&... args) {
        return val.template emplace<1>(list, std::forward<Args>(args)...);
    }
    
    
    constexpr bool has_value() const noexcept { return std::holds_alternative<T>(val); }
    constexpr bool has_error() const noexcept { return std::holds_alternative<Error>(val); }
    
    constexpr explicit operator bool() const noexcept { return has_value(); }
    
    
    T &value() {
        if(auto v = std::get_if<0>(&val))
            return *v;
        throw bad_result_value_access();
    }
    
    const T &value() const {
        if(auto v = std::get_if<0>(&val))
            return *v;
        throw bad_result_value_access();
    }
    
    Error &error() {
        if(auto e = std::get_if<1>(&val))
            return *e;
        throw bad_result_error_access();
    }
    
    const Error &error() const {
        if(auto e = std::get_if<1>(&val))
            return *e;
        throw bad_result_error_access();
    }
    
    
    template<typename U>
    constexpr T value_or(U &&default_value) const {
        if(auto v = std::get_if<0>(&val))
            return *v;
        return std::forward<U>(default_value);
    }
    
    template<typename Func>
    constexpr T value_or_get(Func &&errorFunc) const {
        if(auto v = std::get_if<0>(&val))
            return *v;
        return errorFunc(error());
    }
    
    T &value_or_throw() {
        if(auto v = std::get_if<0>(&val))
            return *v;
        throw std::get<1>(val);
    }
    
    const T &value_or_throw() const {
        if(auto v = std::get_if<0>(&val))
            return *v;
        throw std::get<1>(val);
    }
    
    template<typename Except, typename... Args>
    T &value_or_throw(Args&&... args) {
        if(auto v = std::get_if<0>(&val))
            return *v;
        throw Except(std::forward<Args>(args)...);
    }
    
    template<typename Except, typename... Args>
    const T &value_or_throw(Args&&... args) const {
        if(auto v = std::get_if<0>(&val))
            return *v;
        throw Except(std::forward<Args>(args)...);
    }
    
   
    template<typename ValueFunc>
    auto map(ValueFunc &&func) const {
        auto v = std::get_if<0>(&val);
        using Result = result<decltype(func(*v)), Error>;
        
        if(v)
            return Result(func(*v));
        else
            return Result(error());
    }

    template<typename ValueFunc, typename ErrorFunc>
    auto map(ValueFunc &&valueFunc, ErrorFunc &&errorFunc) const {
        auto v = std::get_if<0>(&val);
        using Result = result<decltype(valueFunc(*v)), decltype(errorFunc(error()))>;
        
        if(v)
            return Result(result_in_place_value(), valueFunc(*v));
        else
            return Result(result_in_place_error(), errorFunc(error()));
    }


    template<typename ValueFunc, typename ErrorFunc>
    void perform(ValueFunc &&value_func, ErrorFunc &&error_func) {
        if(auto v = std::get_if<0>(&val))
            value_func(*v);
        else
            error_func(std::get<1>(val));
    }
    
    template<typename ValueFunc, typename ErrorFunc>
    void perform(ValueFunc &&value_func, ErrorFunc &&error_func) const {
        if(auto v = std::get_if<0>(&val))
            value_func(*v);
        else
            error_func(std::get<1>(val));
    }
    
private:
    std::variant<T, Error> val;
};


} // namespace liph

#endif
