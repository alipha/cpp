#ifndef LIPH_TRAMPOLINE_HPP
#define LIPH_TRAMPOLINE_HPP

#include <functional>
#include <memory>
#include <optional>
#include <deque>
#include <stdexcept>
#include <string>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <vector>


namespace liph {


template<typename DerivedRef, typename Base>
std::enable_if_t<!std::is_pointer_v<DerivedRef>, DerivedRef> down_cast(Base &&base) {
    static_assert(std::is_reference_v<DerivedRef>, 
        "In down_cast<Derived>, Derived must be either a pointer or a reference");
    
    using Derived = std::remove_reference_t<DerivedRef>;
    
    Derived *p = down_cast<Derived*>(&base);
    if(p)
        return *p;
    else
        throw std::bad_cast();
}

template<typename DerivedPtr, typename Base>
std::enable_if_t<std::is_pointer_v<DerivedPtr>, DerivedPtr> down_cast(Base *base) {
    using Derived = std::remove_pointer_t<DerivedPtr>;

    static_assert(std::is_base_of_v<Base, Derived>, "Base must be a base of Derived");
    static_assert(std::has_virtual_destructor_v<Base>, "Base must be a polymorphic type");

    if(typeid(*base) == typeid(Derived))
        return static_cast<DerivedPtr>(base);
    else
        return nullptr;
}



template <size_t... indices, typename T1, typename Func>
auto transform(T1&& s, Func f, std::index_sequence<indices...>)
{
    return std::make_tuple(f(std::get<indices>(std::forward<T1>(s)))...);
}

template <typename T1, typename Func>
auto transform(T1&& s, Func f)
{
    constexpr std::size_t N = std::tuple_size<std::remove_reference_t<T1>>::value;
    return transform(std::forward<T1>(s), f, std::make_index_sequence<N>());
}


namespace detail {


struct task;
using tasks = std::deque<std::unique_ptr<task>>;


struct resolver_base {
    resolver_base() = default;
    resolver_base(const resolver_base &) = delete;
    resolver_base(resolver_base &&) = delete;
    resolver_base &operator=(const resolver_base &) = delete;
    resolver_base &operator=(resolver_base &&) = delete;

    virtual const std::type_info &type() const = 0;
    virtual ~resolver_base() = default;
};


struct task {
    task() = default;
    task(const task &) = delete;
    task(task &&) = delete;
    task &operator=(const task &) = delete;
    task &operator=(task &&) = delete;
    
    virtual ~task() = default;
    virtual bool run(tasks &combiners, tasks &fetchers, resolver_base &r) = 0;
};

}  // namespace detail



struct trampoline_resolver_type_mismatch : public std::logic_error {
    trampoline_resolver_type_mismatch(const std::string &msg) : std::logic_error(msg) {}
};


template<typename Ret>
class trampoline;


template<typename Ret>
class trampoline_list {
public:
    trampoline_list() = default;

    template<typename Func>
    void add(Func && func) { 
        funcs.push_back(std::forward<Func>(func)); 
    }

    std::vector<trampoline<Ret>> operator()() {
        std::vector<trampoline<Ret>> trampolines;
        
        for(auto &func : funcs)
            trampolines.push_back(func());

        return trampolines;
    }

private:
    std::vector<std::function<trampoline<Ret>()>> funcs;
};



template<typename Ret>
class trampoline {
public:
    using result_type = Ret;
    
private:

    struct resolver : detail::resolver_base {
        resolver(std::optional<Ret> *rp) : result_ptr(rp) {}
        ~resolver() = default;

        virtual const std::type_info &type() const override {
            return typeid(Ret);
        }

        void resolve(Ret value) {
            *result_ptr = std::move(value);
        }

        std::optional<Ret> *result_ptr;
    };

        
    struct executor_base {
        executor_base() : result_ptr() {}
        executor_base(const executor_base &) = delete;
        executor_base(executor_base &&) = delete;
        executor_base &operator=(const executor_base &) = delete;
        executor_base &operator=(executor_base &&) = delete;
        
        virtual ~executor_base() = default;
        
        virtual bool run(detail::tasks &combiners, detail::tasks &fetchers, detail::resolver_base &r) = 0;
        
        std::optional<Ret> *result_ptr;
    };
    
    
    template<typename Collector, typename Trampolines>
    struct combiner : detail::task {
        template<typename C, typename T>
        combiner(C &&c, T &&t, std::optional<Ret> *rp) 
            : collect(std::forward<C>(c)), trampolines(std::forward<T>(t)), combined_result(rp) {}
        
        ~combiner() = default;
        
        bool run(detail::tasks &, detail::tasks &, detail::resolver_base &) override {
            auto results = transform(trampolines, transformer());
            *combined_result = std::apply(collect, results); 
            return false;
        }

        struct transformer {
            template<typename T>
            T& operator()(trampoline<T> &t) { return *t.result; }

            template<typename T>
            std::vector<T> operator()(std::vector<trampoline<T>> &v) {
                std::vector<T> results;
                for(auto &t : v)
                    results.push_back(std::move(*t.result));
                return results;
            }
        };

        Collector collect;
        Trampolines trampolines;
        std::optional<Ret> *combined_result;
    };
    
    
    template<typename Trampolines>
    struct fetcher : detail::task {
        fetcher(Trampolines *t) : trampolines(t) {}
        
        ~fetcher() = default;
        
        bool run(detail::tasks &combiners, detail::tasks &fetchers, detail::resolver_base &r) override {
            return std::apply(
                [&](auto& ...t) { 
                    return (this->run_trampoline(t, combiners, fetchers, r) || ...); 
                }, *trampolines);
        }
        
        template<typename T>
        bool run_trampoline(T &t, detail::tasks &combiners, detail::tasks &fetchers, detail::resolver_base &r) {
            if(t.exec) {
                t.exec->result_ptr = &t.result;
                return t.exec->run(combiners, fetchers, r);
            } else {
                return false;
            }
        }
        
        template<typename T>
        bool run_trampoline(std::vector<T> &v, detail::tasks &combiners, detail::tasks &fetchers, detail::resolver_base &r) {
            for(T &t : v) {
                if(run_trampoline(t, combiners, fetchers, r))
                    return true;
            }
            
            return false;
        }
        
        Trampolines *trampolines;
    };

    
    template<typename Collector, typename... Func>
    struct executor : executor_base {
        using trampolines = std::tuple<std::remove_reference_t<decltype(std::declval<Func>()())>...>;
        
        template<typename C, typename... F>
        executor(C &&c, F&&... f) : collector(std::forward<C>(c)), funcs(std::forward<F>(f)...) {}
        
        ~executor() = default;
                
        bool run(detail::tasks &combiners, detail::tasks &fetchers, detail::resolver_base &) override {
            trampolines trams = transform(funcs, [](auto &f) { return f(); });
            auto comb = std::make_unique<combiner<Collector, trampolines>>(std::move(collector), std::move(trams), this->result_ptr);
            auto fetch = std::make_unique<fetcher<trampolines>>(&comb->trampolines);
            
            combiners.push_back(std::move(comb));
            fetchers.push_back(std::move(fetch));
            return false;
        }
        
        Collector collector;
        std::tuple<Func...> funcs;
        std::unique_ptr<fetcher<trampolines>> fetch;
        std::unique_ptr<combiner<Collector, trampolines>> comb;
    };


    template<typename T>
    struct resolver_executor : executor_base {
        template<typename U>
        resolver_executor(U &&v) : value(std::forward<U>(v)) {}

        ~resolver_executor() = default;
                
        bool run(detail::tasks &, detail::tasks &, detail::resolver_base &r) override {
            if(auto p = down_cast<typename trampoline<T>::resolver*>(&r)) {
                p->resolve(std::move(value));
                return true;
            } else {
                throw trampoline_resolver_type_mismatch(std::string("trampoline<T>::resolve<U>() called [with T=")
                    + typeid(Ret).name() + " and U=" + typeid(T).name() + "], but U was expected to be "
                    + r.type().name() + " to match the U in trampoline<U>::run()");
            }
        }

        T value;
    };
    
    
    trampoline(std::unique_ptr<executor_base> e) : result(), exec(std::move(e)) {}
    
public:
    template<typename R>
    trampoline(R &&v) : result(std::forward<R>(v)), exec() {}
        
    template<typename Collector, typename Func1, typename... Func>
    trampoline(Collector &&c, Func1 &&f1, Func&&... f) 
        : result(),
          exec(std::make_unique<executor<std::remove_reference_t<Collector>, std::remove_reference_t<Func1>, std::remove_reference_t<Func>...>>(
                std::forward<Collector>(c), 
                std::forward<Func1>(f1),
                std::forward<Func>(f)...))  {}
    
    Ret run() {
        if(result)
            return std::move(*result);
        
        resolver resolve(&result);
        detail::tasks task_stack;
        exec->result_ptr = &result;
        exec->run(task_stack, task_stack, resolve);
        
        while(!task_stack.empty() && !result) {
            std::unique_ptr<detail::task> t = std::move(task_stack.back());
            task_stack.pop_back();
            t->run(task_stack, task_stack, resolve);
        }
        
        return *result;
    }

    Ret run_breadth() {
        if(result)
            return std::move(*result);
            
        resolver resolve(&result);
        detail::tasks combiner_stack;
        detail::tasks fetcher_queue;
        exec->result_ptr = &result;
        exec->run(combiner_stack, fetcher_queue, resolve);
        
        while(!fetcher_queue.empty() && !result) {
            std::unique_ptr<detail::task> t = std::move(fetcher_queue.front());
            fetcher_queue.pop_front();
            t->run(combiner_stack, fetcher_queue, resolve);
        }
       
        while(!combiner_stack.empty() && !result) {
            std::unique_ptr<detail::task> t = std::move(combiner_stack.back());
            combiner_stack.pop_back();
            t->run(combiner_stack, fetcher_queue, resolve);
        }
        
        return *result;
    }

    static trampoline resolve(Ret value) {
        return trampoline(std::unique_ptr<executor_base>(
                    std::make_unique<resolver_executor<Ret>>(std::move(value))
        ));
    }

    template<typename T, typename U>
    static trampoline resolve(U &&value) {
        return trampoline<Ret>(std::unique_ptr<executor_base>(
                    std::make_unique<resolver_executor<T>>(std::forward<U>(value))
        ));
    }
    
private:
    template<typename T>
    friend class trampoline;

    std::optional<Ret> result;
    std::unique_ptr<executor_base> exec;
};


}  // namespace liph

#endif

