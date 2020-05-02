#ifndef LIPH_CACHED_FUNCTION_HPP
#define LIPH_CACHED_FUNCTION_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <utility>
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
    using call_set = std::set<call_state_t, std::less<>>;

public:
    cached_function(std::size_t max, Functor f) : max_size(max / 2), f(std::move(f)) {}
    cached_function(Functor f) : f(std::move(f)) {}
    
    template<typename... Args>
    typename call_state_t::return_t operator()(Args&&... args) {
        std::tuple<Args&&...> arg_refs{std::forward<Args>(args)...};
        
        auto it = (this->*new_calls).find(arg_refs);

        bool in_new = (it != (this->*new_calls).end());
        if(!in_new)
            it = (this->*old_calls).find(arg_refs);

        if(!in_new && it == (this->*old_calls).end()) {  // if not in either
            call_state_t call(f(std::forward<Args>(args)...), std::forward<Args>(args)...);

            if(fill_new())
                it = (this->*new_calls).insert(std::move(call)).first;
            else
                it = (this->*old_calls).insert(std::move(call)).first;
        } else if(!in_new && fill_new()) {
            // if call needs to be moved from old to new
            call_state_t call = std::move((this->*old_calls).extract(it).value());
            it = (this->*new_calls).insert(std::move(call)).first;
        } // if in new, do nothing. (or in old and doesn't need to be moved) just return it
            
        auto ret = it->get_return_value();

        if((this->*new_calls).size() > max_size)
            expire();

        return ret;
    }

    void set_max_size(std::size_t max) {
        max_size = max / 2;

        if(calls1.size() + calls2.size() > max)
            expire();
    }

    void expire_all() {
        calls1.clear();
        calls2.clear();
    }
   
    // only leave the most recent calls 
    void expire() {
        (this->*old_calls).clear();
        std::swap(old_calls, new_calls);
    }

private:
    bool fill_new() const {
        return !(this->*new_calls).empty() || (this->*old_calls).size() >= max_size;
    }

    std::size_t max_size = 25;
    Functor f;
    call_set calls1;
    call_set calls2;
    call_set cached_function::*new_calls = &cached_function::calls1;
    call_set cached_function::*old_calls = &cached_function::calls2;
};

#endif
