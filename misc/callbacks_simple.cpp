#include <iostream>
#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <string>


struct no_callback_slots_available : std::runtime_error {
    no_callback_slots_available() : std::runtime_error("no callback slots are available.") {}
};

struct invalid_callback : std::logic_error {
    invalid_callback() : std::logic_error("the function pointer supplied does not map to a callback.") {}
};


template<typename Func, std::size_t MaxCount>
struct callbacks {
    static_assert(sizeof(Func) && false, "Func is not a function type");
};

template<typename Ret, typename... Args, std::size_t MaxCount>
struct callbacks<Ret(Args...), MaxCount> {
    using Func = Ret(Args...);

    template<typename Callback>
    static Func *add(Callback &&callback) {
        std::size_t i = getNextIndex();
        funcs[i] = std::forward<Callback>(callback);
        ++used_count;
        return getFuncPtr<>(i);
    }

    static bool remove(Func *func) {
        for(std::size_t i = 0; i < MaxCount; ++i) {
            if(func == getFuncPtr<>(i)) {
                if(funcs[i] == nullptr) {
                    return false;
                } else {
                    --used_count;
                    funcs[i] = nullptr;
                    return true;
                }
            }
        }
        throw invalid_callback();
    }

    static bool is_full() {
        return used_count == MaxCount;
    }

    static std::size_t count() {
        return used_count;
    }
private:
    template<std::size_t Index = 0>
    static Func *getFuncPtr(std::size_t i) {
        if constexpr(Index >= MaxCount) {
            throw std::logic_error("Index is somehow >= MaxCount");
        } else {
            if(i == Index) {
                return [](Args... args) -> Ret { 
                    return funcs[Index](std::forward<Args>(args)...); 
                };
            } else {
                return getFuncPtr<Index+1>(i);
            }
        }
    }

    static std::size_t getNextIndex() {
        auto it = std::find_if(funcs.begin(), funcs.end(), [](auto &f) { return f == nullptr; });
        if(it == funcs.end())
            throw no_callback_slots_available();
        return it - funcs.begin();
    }

    static std::array<std::function<Func>, MaxCount> funcs;
    static std::size_t used_count;
};

template<typename Ret, typename... Args, std::size_t MaxCount>
std::array<std::function<Ret(Args...)>, MaxCount> callbacks<Ret(Args...), MaxCount>::funcs;

template<typename Ret, typename... Args, std::size_t MaxCount>
std::size_t callbacks<Ret(Args...), MaxCount>::used_count;



// allow up for 5 different active callbacks matching signature `int()`
using test_callbacks = callbacks<int(), 5>;


int main() {
    int x = 8;
    int (*f)() = test_callbacks::add([&x]() { return x * 2; });
    int (*g)() = test_callbacks::add([&x]() { return x * 3; });

    test_callbacks::remove(f);
    test_callbacks::add([&x]() { return x * 5; });
    x = 11;
    std::cout << test_callbacks::count() << f() << g() << std::endl;
}

