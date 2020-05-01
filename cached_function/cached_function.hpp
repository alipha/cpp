#ifndef LIPH_CACHED_FUNCTION_HPP
#define LIPH_CACHED_FUNCTION_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <tuple>
#include <type_traits>


namespace detail {

template<typename T>
struct decay_no_ref {
    using type = std::decay_t<T>;
};

template<typename T>
struct decay_no_ref<const T&> {
    using type = std::decay_t<T>;
};

template<typename T>
struct decay_no_ref<T&&> {
    using type = std::decay_t<T>;
};

template<typename T>
struct decay_no_ref<T&> {
    static_assert(false && sizeof(T), "non-const reference parameters are not allowed for cached_function");
};

template<typename T>
using decay_no_ref_t = typename decay_no_ref<T>::type;


template<typename R, typename... Args>
struct call_state {
    using return_t = R;
    using params_t = std::tuple<Args...>;
    using stored_return_t = std::conditional_t<std::is_reference_v<R>, std::remove_reference_t<R>*, R>;
    using stored_params_t = std::tuple<decay_no_ref_t<Args>...>;
    
    template<typename R2, typename... Args2>
    call_state(R2 &&r, Args2&&... args) : return_value(get_return_value(std::forward<R>(r))), params(std::forward<Args>(args)...) {}
    
    
    template<typename U = R>
    std::enable_if_t<std::is_reference_v<U>, R> get_return_value() const { return *return_value; }
    
    template<typename U = R>
    std::enable_if_t<!std::is_reference_v<U>, R> get_return_value() const { return return_value; }
    
    template<typename U = R>
    std::enable_if_t<std::is_reference_v<U>, stored_return_t> get_return_value(R r) const { return &r; }
    
    template<typename U = R>
    std::enable_if_t<!std::is_reference_v<U>, stored_return_t> get_return_value(R r) const { return r; }
       
private:
    stored_return_t return_value;
    
public:
    stored_params_t params;
};

template<typename R, typename... Args>
call_state<R, Args...> make_call_state(R(*)(Args...));

template<typename R, typename T, typename... Args>
call_state<R, Args...> make_call_state(R(T::*)(Args...));

template<typename R, typename T, typename... Args>
call_state<R, Args...> make_call_state(R(T::*)(Args...) const);

template<typename R, typename T, typename... Args>
call_state<R, Args...> make_call_state(R(T::*)(Args...) &);

template<typename R, typename T, typename... Args>
call_state<R, Args...> make_call_state(R(T::*)(Args...) const &);

template<typename Functor>
auto make_call_state(Functor) -> decltype(make_call_state(&Functor::operator()));


/*
template<typename R, typename... Args>
struct call_state_w_counter : call_state<R, Args...> {
    template<typename R2, typename... Args2>
    call_state(std::size_t c, R2 &&r, Args2&&... args) : counter(c), call_state<R, Args...>(std::forward<R>(r), std::forward<Args>(args)...) {}

    std::size_t counter;
};
*/

template<typename R, typename... Args>
bool operator<(const call_state<R, Args...> &left, const call_state<R, Args...> &right) {
    return left.params < right.params;
}

template<typename R, typename... Args, typename... Args2>
bool operator<(const std::tuple<Args2...> &left, const call_state<R, Args...> &right) {
    return left < right.params;
}

template<typename R, typename... Args, typename... Args2>
bool operator<(const call_state<R, Args...> &left, const std::tuple<Args2...> &right) {
    return left.params < right;
}


} // namespace detail



template<typename Functor>
class basic_cached_function {
    using call_state_t = decltype(detail::make_call_state(std::declval<Functor>()));
    
public:
    basic_cached_function(Functor f) : f(std::move(f)), last_call() {}
    
    basic_cached_function(const basic_cached_function &) = delete;
    basic_cached_function(basic_cached_function &&) = delete;
    void operator=(const basic_cached_function &) = delete;
    void operator=(basic_cached_function &&) = delete;
    
    template<typename... Args>
    typename call_state_t::return_t operator()(Args&&... args) {
        std::tuple<Args&&...> arg_refs{std::forward<Args>(args)...};
        
        if(!last_call || last_call->params != arg_refs)
            last_call.emplace(f(std::forward<Args>(args)...), std::forward<Args>(args)...);
            
        return last_call->get_return_value();
    }
    
private:
    Functor f;
    std::optional<call_state_t> last_call;
};



template<typename Functor>
class cached_function {
    using call_state_t = decltype(detail::make_call_state(std::declval<Functor>()));
    
public:
    cached_function(std::size_t max, Functor f) : max_size(max), counter(0), f(std::move(f)), calls() {}
    cached_function(Functor f) : max_size(50), counter(0), f(std::move(f)), calls() {}
    
    cached_function(const cached_function &) = delete;
    cached_function(cached_function &&) = delete;
    void operator=(const cached_function &) = delete;
    void operator=(cached_function &&) = delete;
    
    template<typename... Args>
    typename call_state_t::return_t operator()(Args&&... args) {
        std::tuple<Args&&...> arg_refs{std::forward<Args>(args)...};
        
        auto it = calls.find(arg_refs);

        if(it == calls.end()) {
            call_state_t call(f(std::forward<Args>(args)...), std::forward<Args>(args)...);
            it = calls.try_emplace(call, counter++).first;

            if(calls.size() > max_size)
                expire();
        } else if(it->second < counter - max_size / 4) {
            it->second = counter++;
        }
            
        return it->first.get_return_value();
    }

    void set_max_size(std::size_t max) {
        max_size = max;

        if(calls.size() > max_size)
            expire();
    }

    void expire_all() {
        calls.clear();
        counter = 0;
    }
   
    // only leave the most recent calls 
    void expire() {
        std::int64_t cutoff = counter - max_size / 2;

        for(auto it = calls.begin(); it != calls.end(); ) {
            if(it->second < cutoff)
                it = calls.erase(it);
            else
                ++it;
        }
    }

private:
    std::size_t max_size;
    std::int64_t counter;
    Functor f;
    std::map<call_state_t, std::int64_t, std::less<>> calls;
};

#endif
