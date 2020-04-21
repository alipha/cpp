#ifndef LIPH_CACHED_FUNCTION_HPP
#define LIPH_CACHED_FUNCTION_HPP

#include <optional>
#include <tuple>
#include <type_traits>


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
    std::enable_if_t<std::is_reference_v<U>, R> get_return_value() { return *return_value; }
    
    template<typename U = R>
    std::enable_if_t<!std::is_reference_v<U>, R> get_return_value() { return return_value; }
    
    template<typename U = R>
    std::enable_if_t<std::is_reference_v<U>, stored_return_t> get_return_value(R r) { return &r; }
    
    template<typename U = R>
    std::enable_if_t<!std::is_reference_v<U>, stored_return_t> get_return_value(R r) { return r; }
       
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



template<typename Functor>
class cached_function {
    using call_state_t = decltype(make_call_state(std::declval<Functor>()));
    
public:
    cached_function(Functor f) : f(std::move(f)), last_call() {}
    
    cached_function(const cached_function &) = delete;
    cached_function(cached_function &&) = delete;
    void operator=(const cached_function &) = delete;
    void operator=(cached_function &&) = delete;
    
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

#endif
