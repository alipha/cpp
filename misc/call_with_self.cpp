#include <iostream>
#include <functional>
#include <utility>
#include <thread>
#include <chrono>
#include <type_traits>


template<typename Func>
struct call_with_self {
    call_with_self(Func f) : func(std::move(f)) {}

    template<typename... Args>
    auto operator()(Args&&... args) const {
        return func(func, std::forward<Args>(args)...);
    }

    Func func;
};


template<typename Func>
void run_after_ms(Func func, std::size_t ms) {
    std::thread t([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        func();
    });
    t.detach();
}


int main() {
    int counter = 10;

    run_after_ms(call_with_self([&](auto &self) {
        std::cout << "called with " << counter-- << std::endl;
        if(counter > 0)
            run_after_ms(call_with_self(self), 10);
        else
            std::exit(0);
    }), 10);

    while(true) {
        std::cout << "tick\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
