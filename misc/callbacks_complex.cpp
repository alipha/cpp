#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <stdexcept>


struct no_callback_slots_available : std::runtime_error {
    no_callback_slots_available() : std::runtime_error("no callback slots are available.") {}
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

        Func *p = getFuncPtr<>(i);
        funcToIndex[p] = i;
        return p;
    }

    bool remove(Func *func) {
        auto it = funcToIndex.find(func);
        if(it == funcToIndex.end()) {
            return false;
        }

        std::size_t index = it->second;
        funcs.erase(index);
        funcToIndex.erase(it);

        if(!funcs.empty()) {
            std::size_t maxIndex = funcs.rbegin()->first;
            auto it = unusedIndexes.lower_bound(maxIndex);
            unusedIndexes.erase(it, unusedIndexes.end());
        } else {
            unusedIndexes.clear();
        }

        return true;
    }

    static bool is_full() {
        return funcs.size() == MaxCount;
    }

    static std::size_t count() {
        return funcs.size();
    }
private:
    template<std::size_t Index = 0>
    Func *getFuncPtr(std::size_t i) {
        if constexpr(Index >= MaxCount) {
            throw std::logic_error("Index is somehow >= MaxCount");
        }

        if(i == Index) {
            return [](Args... args) -> Ret { 
                return funcs[Index](std::forward<Args>(args)...); 
            };
        } else {
            return getFuncPtr<Index+1>(i);
        }
    }

    static std::size_t getNextIndex() {
        if(!unusedIndexes.empty()) {
            std::size_t index = *unusedIndexes.begin();
            unusedIndexes.erase(unusedIndexes.begin());
            return index;
        }

        std::size_t nextIndex = funcs.empty() ? 0 : funcs.rbegin()->first + 1;
        if(nextIndex < MaxCount) {
            return nextIndex;
        }

        throw no_callback_slots_available();
    }

    static std::map<std::size_t, std::function<Func>> funcs;
    static std::unordered_map<Func*, std::size_t> funcToIndex;
    static std::set<std::size_t> unusedIndexes;
};
