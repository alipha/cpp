#include <vector>
#include <memory>
#include <iostream>
#include <functional>
#include <tuple>


template<std::size_t... Indexes>
struct index_sequence {};

template<typename Seq, std::size_t N> 
struct append_to_seq {};

template<std::size_t N, std::size_t... Indexes>
struct append_to_seq<index_sequence<Indexes...>, N> {
    using type = index_sequence<Indexes..., N>;
};

template<std::size_t N>
struct make_index_sequence_impl {
    using type = typename append_to_seq<typename make_index_sequence_impl<N-1>::type, N-1>::type;
};

template<>
struct make_index_sequence_impl<0> {
    using type = index_sequence<>;
};

template<std::size_t N>
auto make_index_sequence() -> typename make_index_sequence_impl<N>::type { return {}; }



template<typename Func>
void for_each_param(Func &&func) {}

template<typename Func, typename FirstArg, typename... RestArgs>
void for_each_param(Func &&func, FirstArg&& first, RestArgs&&... rest) {
    func(std::forward<FirstArg>(first));
    for_each_param(std::forward<Func>(func), std::forward<RestArgs>(rest)...);
}

template<typename Func, typename... Args, std::size_t... Indexes>
void apply_impl(Func &&func, std::tuple<Args...> t, index_sequence<Indexes...>) {
    for_each_param(func, std::get<Indexes>(t)...);
}

template<typename Func, typename... Args>
void apply(Func &&func, std::tuple<Args...> t) {
    apply_impl(func, t, make_index_sequence<sizeof...(Args)>());
}



struct print {
    template<typename T>
    void operator()(const T &t) const {
        std::cout << t << '\n';
    }
};


int main() { 
    std::tuple<int, float, int, char> t{3, 5.1, 7, 'x'};
    apply(print{}, t); 
} 

