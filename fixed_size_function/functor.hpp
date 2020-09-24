#ifndef LIPH_FUNCTOR_HPP
#define LIPH_FUNCTOR_HPP

#include <cstddef>
#include <functional>
#include <new>
#include <utility>


namespace liph {

namespace detail {
    template<typename Ret, typename... Args>
    struct func_behavior {
        virtual Ret call(unsigned char *, Args...) { throw std::bad_function_call(); }
        virtual void move(unsigned char *, unsigned char *) noexcept {}
        virtual void destroy(unsigned char *) noexcept {}
        
        static func_behavior instance;
    };
    
    template<typename Ret, typename... Args>
    func_behavior<Ret, Args...> func_behavior<Ret, Args...>::instance;
    
    
    template<typename Func, typename Ret, typename... Args>
    struct func_behavior_impl : func_behavior<Ret, Args...> {
        Ret call(unsigned char *storage, Args... args) override {
            return get(storage)(std::forward<Args>(args)...);
        }
        
        void move(unsigned char *src, unsigned char *dest) noexcept override {
            new (dest) Func(std::move(get(src)));
        }
        
        void destroy(unsigned char *storage) noexcept override { get(storage).~Func(); }

        static func_behavior_impl instance;
        
    private:
        Func &get(unsigned char *storage) noexcept { return *reinterpret_cast<Func*>(storage); }
    };
    
    template<typename Func, typename Ret, typename... Args>
    func_behavior_impl<Func, Ret, Args...> func_behavior_impl<Func, Ret, Args...>::instance;
}


template<typename Func, std::size_t MaxSize = sizeof(Func*)>
struct functor {
    static_assert(sizeof(Func) && false, "Func is not a function type");
};

template<typename Ret, typename... Args, std::size_t MaxSize>
struct functor<Ret(Args...), MaxSize> {
    using result_type = Ret;
    
    functor() noexcept : behavior(&detail::func_behavior<Ret, Args...>::instance) {}
    functor(std::nullptr_t) noexcept : functor() {}
    
    template<typename Func>
    functor(Func func) : behavior(&detail::func_behavior_impl<Func, Ret, Args...>::instance) {
        static_assert(sizeof(Func) <= MaxSize, "Func is too large to store in functor");
        new (storage) Func(std::move(func));
    }
    
    functor(functor &&other) noexcept : behavior(other.behavior) {
        behavior->move(other.storage, storage);
        behavior->destroy(other.storage);
        other.behavior = &detail::func_behavior<Ret, Args...>::instance;
    }
    
    functor &operator=(functor &&other) noexcept {
        behavior->destroy(storage);
        behavior = other.behavior;
        behavior->move(other.storage, storage);
        behavior->destroy(other.storage);
        return *this;
    }
    
    ~functor() { behavior->destroy(storage); }
    
    Ret operator()(Args... args) const { 
        return behavior->call(storage, std::forward<Args>(args)...);
    }
    
    operator bool() const noexcept { return behavior == &detail::func_behavior<Ret, Args...>::instance; }
    
private:
    alignas(alignof(std::max_align_t)) mutable unsigned char storage[MaxSize];
    detail::func_behavior<Ret, Args...> *behavior;
};


} // namespace liph

#endif
