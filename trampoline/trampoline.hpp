#ifndef LIPH_TRAMPOLINE_HPP
#define LIPH_TRAMPOLINE_HPP

#include <memory>
#include <optional>
#include <stack>
#include <tuple>
#include <utility>


namespace liph {


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

struct task {
    task() = default;
    task(const task &) = delete;
    task(task &&) = delete;
    task &operator=(const task &) = delete;
    task &operator=(task &&) = delete;
    
    virtual ~task() = default;
    virtual void run(std::stack<std::unique_ptr<task>> &task_stack) = 0;
};

}  // namespace detail


template<typename Ret>
class trampoline {
public:
    using result_type = Ret;
    
private:
        
    struct executor_base {
        executor_base() : result_ptr() {}
        executor_base(const executor_base &) = delete;
        executor_base(executor_base &&) = delete;
        executor_base &operator=(const executor_base &) = delete;
        executor_base &operator=(executor_base &&) = delete;
        
        virtual ~executor_base() = default;
        
        virtual void run(std::stack<std::unique_ptr<detail::task>> &task_stack) = 0;
        
        std::optional<Ret> *result_ptr;
    };
    
    
    template<typename Collector, typename Trampolines>
    struct combiner : detail::task {
        template<typename C, typename T>
        combiner(C &&c, T &&t, std::optional<Ret> *rp) : collect(std::forward<C>(c)), trampolines(std::forward<T>(t)), combined_result(rp) {}
        
        ~combiner() = default;
        
        void run(std::stack<std::unique_ptr<detail::task>> &) override {
            auto results = transform(trampolines, [](auto &t) { return *t.result; });
            **combined_result = std::apply(collect, results); 
        }

        Collector collect;
        Trampolines trampolines;
        std::optional<Ret> *combined_result;
    };
    
    
    template<typename Trampolines>
    struct fetcher : detail::task {
        fetcher(Trampolines *t) : trampolines(t) {}
        
        ~fetcher() = default;
        
        void run(std::stack<std::unique_ptr<detail::task>> &task_stack) override {
            std::apply([&task_stack, this](auto& ...t){ (..., this->run_trampoline(t, task_stack)); }, *trampolines);
        }
        
        template<typename T>
        void run_trampoline(T &t, std::stack<std::unique_ptr<detail::task>> &task_stack) {
            if(t.exec) {
                t.exec->result_ptr = &t.result;
                t.exec->run(task_stack);
            }
        }
        
        Trampolines *trampolines;
    };

    
    template<typename Collector, typename... Func>
    struct executor : executor_base {
        using trampolines = std::tuple<std::remove_reference_t<decltype(std::declval<Func>()())>...>;
        
        template<typename C, typename... F>
        executor(C &&c, F&&... f) : collector(std::forward<C>(c)), funcs(std::forward<F>(f)...) {}
        
        ~executor() = default;
                
        void run(std::stack<std::unique_ptr<detail::task>> &task_stack) override {
            trampolines trams = transform(funcs, [](auto &f) { return f(); });
            auto comb = std::make_unique<combiner<Collector, trampolines>>(std::move(collector), std::move(trams), this->result_ptr);
            auto fetch = std::make_unique<fetcher<trampolines>>(&comb->trampolines);
            
            task_stack.push(std::move(comb));
            task_stack.push(std::move(fetch));
        }
        
        Collector collector;
        std::tuple<Func...> funcs;
        std::unique_ptr<fetcher<trampolines>> fetch;
        std::unique_ptr<combiner<Collector, trampolines>> comb;
    };
    
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
            
        std::stack<std::unique_ptr<detail::task>> task_stack;
        exec->result_ptr = &result;
        exec->run(task_stack);
        
        while(!task_stack.empty()) {
            std::unique_ptr<detail::task> t = std::move(task_stack.top());
            task_stack.pop();
            t->run(task_stack);
        }
        
        return *result;
    }
    
private:
    std::optional<Ret> result;
    std::unique_ptr<executor_base> exec;
};


}  // namespace liph

#endif

