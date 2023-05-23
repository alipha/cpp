#include <iostream>
#include <vector>

#include <utility>


template<std::size_t N, typename Container, typename T, typename Func, std::size_t... Indexes>
void try_invoke_with_n_args(Container &&c, T &result, Func &&f, std::index_sequence<Indexes...>) {
    if(N == c.size()) {
        result = f(c[Indexes]...);
    }
}

template<std::size_t Min, typename Container, typename T, typename Func, std::size_t... Sizes>
T invoke_expand_impl(Container &&c, T &&default_value, Func &&f, std::index_sequence<Sizes...>) {
    using NonConstT = std::decay_t<T>;
    NonConstT result = std::forward<T>(default_value);
    int hack[] = {(try_invoke_with_n_args<Min+Sizes>(c, result, f, std::make_index_sequence<Min+Sizes>()), 0)...};
    return result;
}

template<std::size_t Min, std::size_t Max, typename Container, typename T, typename Func>
T invoke_expand(Container &&c, T &&default_value, Func &&f) {
    return invoke_expand_impl<Min>(c, std::forward<T>(default_value), f, std::make_index_sequence<Max-Min+1>());
}


template<typename... Args>
int print(Args&&... args) {
    std::cout << "Printing " << sizeof...(args) << " args\n";
    int hack[] = {(std::cout << args << ' ', 0)...};
    std::cout << "Done\n";
    return sizeof...(args) * 100;
}


void call_print(const std::vector<int> &v) {
    int result = invoke_expand<1, 10>(v, -1,
        [](auto&&... args) {
            return print(args...);
        }
    );
    std::cout << result << '\n';
}

int main() {
    std::vector<int> v{3, 5, 1, 25};
    call_print(v);
    v.resize(11);
    call_print(v);
    v.clear();
    call_print(v);

}
